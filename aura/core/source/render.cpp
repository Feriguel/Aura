// ========================================================================== //
// File : render.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/render.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
#include "Render/ray-tracer.hpp"
// Standard includes.
#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#pragma warning(disable : 26812)
#include <glm/glm.hpp>
#pragma warning(default : 26812)

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the base core and starts rendering.
	/// </summary>
	Render::Render(Nucleus & nucleus) :
		core_nucleus(nucleus),
		stage_flags({ vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe })
	{
		initVulkan();
		queryPhysicalDevices();
	}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Render::~Render() noexcept
	{
		terminateVulkan();
	}

	// ------------------------------------------------------------------ //
	// Render control.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the base core and starts rendering.
	/// </summary>
	bool Render::dispatchFrame()
	{
		std::uint32_t frame_idx { 0U };

		// Wait for previous image to render.
		if(!waitForMainFence(0U))
		{
			return false;
		}
		device.resetFences(1U, &main_fence, dispatch);
		// Try to acquire frame.
		if(!framework->acquireframe(acquisition_semaphore, nullptr, 0, frame_idx))
		{
			return false;
		}
		// Enqueue environment update jobs and wait for them to finish.
		bool update = updateEnvironment(frame_idx);
		// Dispatch all work necessary for this frame render.
		dispatchFrameJobs(frame_idx, update);
		// Set image for display.
		framework->displayFrame(1U, &dispatch_jobs[dispatch_jobs.size() - 1].c_semaphore, frame_idx, present.queue);
		return true;
	}
	/// <summary>
	/// Waits for the device to complete all it's given tasks, must be used
	/// before destroying any of the tear-down functions.
	/// </summary>
	void Render::waitIdle() const noexcept
	{
		vk::Result const result { device.waitIdle(dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "Wait idle."); }
	}
	/// <summary
	/// Waits for the main fence until all its tasks are finished.
	/// </summary>
	bool Render::waitForMainFence(std::uint64_t timeout) const
	{
		vk::Result result { device.waitForFences(1U, &main_fence, static_cast<vk::Bool32>(true), timeout, dispatch) };
		if(result == vk::Result::eNotReady || result == vk::Result::eTimeout)
		{
			return false;
		}
		if(result != vk::Result::eSuccess)
		{
			vk::throwResultException(result, "Present layout wait.");
		}
		return true;
	}
	/// <summary>
	/// Checks for any updates in the environment and clones the new states
	/// to the GPU. Waits for any required work to finish.
	/// </summary>
	bool Render::updateEnvironment(std::uint32_t const & frame_idx) const
	{
		bool update = false;
		constexpr std::size_t n_jobs { 2U };
		constexpr std::size_t n_return_jobs { 2U };
		std::array<std::future<void>, n_jobs> jobs {};
		std::array<std::future<bool>, n_jobs> return_jobs {};

		std::uint32_t const n_samples = core_nucleus.display_settings.anti_aliasing == 0 
			? 1 : core_nucleus.display_settings.anti_aliasing;
		std::uint32_t const n_bounces = core_nucleus.display_settings.ray_depth;
		float const t_min = core_nucleus.display_settings.t_min;
		float const t_max = core_nucleus.display_settings.t_max;

		// Enqueue update jobs.
		jobs[0U] = core_nucleus.enqueue([&] { framework->updateChainImageSet(frame_idx); });
		jobs[1U] = core_nucleus.enqueue([&] { framework->updateRenderSettings(t_min, t_max, n_samples, n_bounces); });
		return_jobs[0U] = core_nucleus.enqueue([&] { return framework->updateRayLauncher(); });
		return_jobs[1U] = core_nucleus.enqueue([&] { return framework->updateScene(); });
		// Wait for jobs to finish.
		for(std::size_t i { 0U }; i < n_jobs; ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
		for(std::size_t i { 0U }; i < n_jobs; ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			update |= return_jobs[i].get();
		}
		return update;
	}
	/// <summary>
	/// Records and submits all necessary commands to render the image in
	/// the current settings.
	/// </summary>
	void Render::dispatchFrameJobs(std::uint32_t const & frame_idx, bool const & update) const
	{
		bool is_random = core_nucleus.display_settings.anti_aliasing != 0U;
		std::size_t n_submits { dispatch_jobs.size() };
		std::vector<std::future<void>> jobs;
		std::vector<vk::SubmitInfo> submits;

		// Enqueue thread records and submit info.
		jobs.resize(n_submits + 1U);
		jobs[0U] = core_nucleus.enqueue([&] { recordPreProcess(frame_idx, update); });
		for(std::size_t i { 1U }; i < n_submits - 1U; ++i)
		{
			jobs[i] = core_nucleus.enqueue([&, is_random, i] { recordSample(is_random, i); });
		}
		jobs[n_submits - 1U] = core_nucleus.enqueue([&] { recordPostProcess(frame_idx); });
		jobs[n_submits] = core_nucleus.enqueue([&] { dispatchSubmitInfo(n_submits, submits); });
		// Wait for thread to finish records and submit info.
		for(std::size_t i { 0U }; i < n_submits + 1U; ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
		// Submit commands.
		compute.queue.submit(static_cast<std::uint32_t>(n_submits), submits.data(), main_fence, dispatch);
	}

	// ------------------------------------------------------------------ //
	// Command recording and submission schedule.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Builds the entire submit info as necessary for queue submission.
	/// </summary>
	void Render::dispatchSubmitInfo(const std::size_t n_submits, std::vector<vk::SubmitInfo> & submits) const
	{
		submits.resize(n_submits);
		submits[0U].setWaitSemaphoreCount(1U).setPWaitSemaphores(&acquisition_semaphore);
		submits[0U].setPWaitDstStageMask(&stage_flags[1U]);
		submits[0U].setCommandBufferCount(1U).setPCommandBuffers(&dispatch_jobs[0U].c_buffer);
		submits[0U].setSignalSemaphoreCount(1U).setPSignalSemaphores(&dispatch_jobs[0U].c_semaphore);
		std::size_t last_idx = 0U;
		for(std::size_t i { 1U }; i < n_submits; ++i)
		{
			submits[i].setWaitSemaphoreCount(1U).setPWaitSemaphores(&dispatch_jobs[last_idx].c_semaphore);
			submits[i].setPWaitDstStageMask(&stage_flags[1U]);
			submits[i].setCommandBufferCount(1U).setPCommandBuffers(&dispatch_jobs[i].c_buffer);
			submits[i].setSignalSemaphoreCount(1U).setPSignalSemaphores(&dispatch_jobs[i].c_semaphore);
			++last_idx;
		}
	}
	/// <summary>
	/// Records the layout transition to geral to a initial submission.
	/// </summary>
	void Render::recordPreProcess(std::uint32_t const frame_idx, bool const update) const
	{
		DispatchJobs const & pre_process { dispatch_jobs[0U] };

		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, pre_process.c_buffer);
		framework->recordChainImageLayoutTransition(frame_idx,
			{}, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
			compute.family, compute.family, stage_flags[1U], stage_flags[1U], pre_process.c_buffer);
		if(update)
		{
			framework->recordPreProcess(pre_process.c_buffer);
			framework->recordVertex(pre_process.c_buffer);
		}
		endRecord(pre_process.c_buffer);
	}
	/// <summary>
	/// Records a sample sequence in the buffer associated with the sample
	/// index. Each sequence includes a ray-generation and x sets of 
	/// intersect, colour and scatter, in this order, equal to the maximum
	/// depth.
	/// </summary>
	void Render::recordSample(bool const is_random, std::size_t const sample_idx) const
	{
		DispatchJobs const & sample { dispatch_jobs[sample_idx] };
		std::uint32_t const n_bounces { core_nucleus.display_settings.ray_depth };
		RandomSeed rnd_seed {};
		RandomPointInCircleAndSeed rnd_point {};

		// Record submission.
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, sample.c_buffer);
		if(is_random) { fillRandoms(rnd_seed); }
		framework->recordRayGen(rnd_seed, sample.c_buffer);
		if(n_bounces != 0U)
		{
			for(std::uint32_t i { 0U }; i < n_bounces - 1U; ++i)
			{
				framework->recordIntersect(sample.c_buffer);
				fillRandomsWithinCircle(rnd_point);
				framework->recordColourAndScatter(rnd_point, sample.c_buffer);
			}
			framework->recordIntersect(sample.c_buffer);
			fillRandomsWithinCircle(rnd_point);
			framework->recordColourAndScatter(rnd_point, sample.c_buffer);
		}
		endRecord(sample.c_buffer);
	}
	/// <summary>
	/// Records both the post-process and the layout transition, in this order,
	/// to the same submission.
	/// </summary>
	void Render::recordPostProcess(std::uint32_t const frame_idx) const
	{
		DispatchJobs const & post_process { dispatch_jobs[dispatch_jobs.size() - 1U] };

		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, post_process.c_buffer);
		framework->recordPostProcess(post_process.c_buffer);
		framework->recordChainImageLayoutTransition(frame_idx,
			vk::AccessFlagBits::eShaderWrite, {}, vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
			compute.family, present.family, stage_flags[1], stage_flags[3], post_process.c_buffer);
		endRecord(post_process.c_buffer);
	}
	/// <summary>
	/// Starts the command buffer record operation.
	/// </summary>
	void Render::beginRecord(vk::CommandBufferUsageFlags const & flags,
		vk::CommandBufferInheritanceInfo const & inheritance_info,
		vk::CommandBuffer const & command) const
	{
		vk::CommandBufferBeginInfo const begin_info(flags, &inheritance_info);
		vk::Result const result { command.begin(&begin_info, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "Command::Begin"); }
	}
	/// <summary>
	/// Ends the command buffer record operation.
	/// </summary>
	void Render::endRecord(vk::CommandBuffer const & command) const
	{
		vk::Result const result { command.end(dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "Command::End"); }
	}
	/// <summary>
	/// Randomizes a value limited to a circle with 1 unit radius.
	/// </summary>
	glm::vec3 Render::randomInCircle() const
	{
		glm::vec3 rand { 0.0, 0.0, 0.0 };
		do
		{
			rand = glm::vec3(core_nucleus.gen(), core_nucleus.gen(), core_nucleus.gen());
		}
		while(glm::dot(rand, rand) > 1.0);
		return rand;
	}
	/// <summary>
	/// Fills structure with a set of a random point and seed.
	/// </summary>
	void Render::fillRandomsWithinCircle(RandomPointInCircleAndSeed & randoms) const
	{
		randoms.point = randomInCircle();
		randoms.seed = core_nucleus.gen();
	}
	/// <summary>
	/// Fills structure with generated values.
	/// </summary>
	void Render::fillRandoms(RandomSeed & randoms) const
	{
		randoms.seed = glm::vec2(core_nucleus.gen(), core_nucleus.gen());
	}

	// ------------------------------------------------------------------ //
	// Vulkan set-up and tear-down related.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets up vulkan by initialising the dynamic dispatcher, instance and
	/// debug messaging (if debug).
	/// It doesn't verify if extensions or layers are supported, as the render
	/// wouldn't work without them.
	/// Throws any error that might occur.
	/// </summary>
	void Render::initVulkan()
	{
		// Initialises instance and dynamic dispatch for that instance.
		{
			// Get instance create address.
			auto const pfnCrtInstance { reinterpret_cast<PFN_vkCreateInstance>(
				glfwGetInstanceProcAddress(nullptr, "vkCreateInstance")) };
			// List of activated layers.
			std::vector<char const *> layers {};
			if constexpr(debug_mode)
			{
				layers.emplace_back("VK_LAYER_KHRONOS_validation");
				if constexpr(DebugSettings::api_dump)
				{ layers.emplace_back("VK_LAYER_LUNARG_api_dump"); }
			}
			// List of activated extensions.
			std::vector<char const *> extensions {};
			{
				if constexpr(debug_mode)
				{ extensions.emplace_back("VK_EXT_debug_utils"); }

				std::uint32_t ext_count { 0U };
				char const ** req_ext {
					glfwGetRequiredInstanceExtensions(&ext_count) };
				for(std::uint32_t i { 0U }; i < ext_count; ++i)
				{ extensions.emplace_back(req_ext[i]); }
			}
			Info const & app_info { core_nucleus.app_info };
			Info const engine_info { getEngineInfo() };
			// Application, engine and vulkan version info.
			vk::ApplicationInfo const info {
				app_info.name.c_str(), makeVulkanVersion(app_info),
				engine_info.name.c_str(), makeVulkanVersion(engine_info),
				VK_MAKE_VERSION(1, 1, 0) };
			// Creates the vulkan instance an checks result.
			vk::InstanceCreateInfo const create_info { {}, &info,
				static_cast<uint32_t>(layers.size()), layers.data(),
				static_cast<uint32_t>(extensions.size()), extensions.data() };
			vk::Result const result { static_cast<vk::Result>(
				pfnCrtInstance(
					reinterpret_cast<VkInstanceCreateInfo const *>(&create_info),
					nullptr,
					reinterpret_cast<VkInstance *>(&instance))) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createInstance"); }
		}
		// Update dynamic dispatcher.
		{
			auto const pfnInstanceProc { reinterpret_cast<PFN_vkGetInstanceProcAddr>(
				glfwGetInstanceProcAddress(instance, "vkGetInstanceProcAddr")) };
			dispatch.init(instance, pfnInstanceProc);
		}
		// Initialises the debug messenger.
		if constexpr(debug_mode)
		{
			vk::DebugUtilsMessageSeverityFlagsEXT const severity_flags {
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError };
			vk::DebugUtilsMessageTypeFlagsEXT const type_flags {
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance };
			// Creates debug messenger and checks result.
			vk::DebugUtilsMessengerCreateInfoEXT const create_info { {},
				severity_flags, type_flags, Render::vulkanDebugCallback, nullptr };
			vk::Result const result { instance.createDebugUtilsMessengerEXT(
				&create_info, nullptr, &debug_messeger, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createDebugUtilsMessengerEXT"); }
		}
	}
	/// <summary>
	/// Tears down vulkan by destroying both created instance and debug
	/// messaging.
	/// </summary>
	void Render::terminateVulkan() noexcept
	{
		if constexpr(debug_mode)
		{
			instance.destroyDebugUtilsMessengerEXT(debug_messeger, nullptr, dispatch);
		}
		instance.destroy(nullptr, dispatch);
	}
	/// <summary>
	/// Creates a compute command pool with the given flags.
	/// Throws any error that might occur.
	/// </summary>
	void Render::createCommandPool(vk::CommandPoolCreateFlags const & flags,
		std::uint32_t const & family, vk::CommandPool & pool) const
	{
		vk::CommandPoolCreateInfo const create_info { flags, family };
		vk::Result const result { device.createCommandPool(
			&create_info, nullptr, &pool, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "createCommandPool"); }
	}
	/// <summary>
	/// Destroys command pool.
	/// </summary>
	void Render::destroyCommandPool(vk::CommandPool & pool) noexcept
	{
		device.destroyCommandPool(pool, nullptr, dispatch);
	}
	/// <summary>
	/// Allocates N buffers at the target data pointer on the given command
	/// pool with intended level.
	/// Throws any error that might occur.
	/// </summary>
	void Render::allocCommandBuffers(
		vk::CommandPool const & pool, vk::CommandBufferLevel const & level,
		std::uint32_t const & n_buffers, vk::CommandBuffer * const p_buffers) const
	{
		vk::CommandBufferAllocateInfo const alloc_info { pool, level, n_buffers };
		vk::Result const result { device.allocateCommandBuffers(
			&alloc_info, p_buffers, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "allocateCommandBuffers"); }
	}
	/// <summary>
	/// Frees N buffers at the target data pointer on the given command pool.
	/// </summary>
	void Render::freeCommandBuffers(vk::CommandPool const & pool,
		std::uint32_t const & n_buffers, vk::CommandBuffer * const p_buffers) noexcept
	{
		device.freeCommandBuffers(pool, n_buffers, p_buffers, dispatch);
	}
	/// <summary>
	/// Creates a semaphore.
	/// Throws any error that might occur.
	/// </summary>
	void Render::createSemaphore(vk::SemaphoreCreateFlags const & flags, vk::Semaphore & semaphore) const
	{
		vk::SemaphoreCreateInfo const create_info { flags };
		vk::Result const result { device.createSemaphore(
			&create_info, nullptr, &semaphore, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "createSemaphore"); }
	}
	/// <summary>
	/// Destroys a semaphore.
	/// </summary>
	void Render::destroySemaphore(vk::Semaphore & semaphore) noexcept
	{
		device.destroySemaphore(semaphore, nullptr, dispatch);
	}
	/// <summary>
	/// Create a fence.
	/// Throws any error that might occur.
	/// </summary>
	void Render::createFence(vk::FenceCreateFlags const & flags, vk::Fence & fence) const
	{
		vk::FenceCreateInfo const create_info { flags };
		vk::Result const result { device.createFence(
			&create_info, nullptr, &fence, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "createFence"); }
	}
	/// <summary>
	/// Destroys a fence.
	/// </summary>
	void Render::destroyFence(vk::Fence & fence) noexcept
	{
		device.destroyFence(fence, nullptr, dispatch);
	}
	/// <summary>
	/// Creates the vulkan surface for the current window.
	/// Throws any error that might occur.
	/// </summary>
	void Render::createSurface()
	{
		vk::Result result { static_cast<vk::Result>(glfwCreateWindowSurface(
			static_cast<VkInstance>(instance), core_nucleus.ui.window,
			nullptr, reinterpret_cast<VkSurfaceKHR *>(&surface))) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "glfwCreateWindowSurface"); }
	}
	/// <summary>
	/// Destroys the currently created vulkan surface. Must be used after any
	/// changes to the window properties.
	/// </summary>
	void Render::destroySurface() noexcept
	{
		instance.destroySurfaceKHR(surface, nullptr, dispatch);
	}
	/// <summary>
	/// Creates the vulkan logic device from the selected physical device
	/// as well as all the queues. Also updates the dispatcher for the new
	/// device.
	/// </summary>
	void Render::createDevice()
	{
		vk::PhysicalDevice const & selected_device { physical_devices[device_index].first };

		std::vector<vk::QueueFamilyProperties> properties {};
		std::uint32_t n_properties { 0U };
		selected_device.getQueueFamilyProperties(&n_properties, nullptr, dispatch);
		properties.resize(n_properties);
		selected_device.getQueueFamilyProperties(&n_properties, properties.data(), dispatch);

		std::vector<std::vector<float>> priorities { n_properties };
		std::vector<vk::DeviceQueueCreateInfo> queue_infos { n_properties };

		std::uint32_t compute_index { 0U };
		typeFamily(properties, vk::QueueFlagBits::eCompute, compute.family);
		addQueueToCreateInfo(compute.family, queue_infos, priorities, compute_index);
		std::uint32_t transfer_index { 0U };
		typeFamily(properties, vk::QueueFlagBits::eTransfer, transfer.family, true);
		addQueueToCreateInfo(transfer.family, queue_infos, priorities, transfer_index);
		std::uint32_t present_index { 0U };
		presentFamily(properties, selected_device, present.family);
		addQueueToCreateInfo(present.family, queue_infos, priorities, present_index);

		for(std::uint32_t i { 0U }; i < n_properties; ++i)
		{
			queue_infos[i]
				.setQueueFamilyIndex(i)
				.setPQueuePriorities(priorities[i].data());
		}
		std::uint32_t empty_families { 0U };
		for(std::uint32_t i { 0U }; i < n_properties - empty_families; ++i)
		{
			if(queue_infos[i].queueCount == 0)
			{
				++empty_families;
				std::swap(queue_infos[i], queue_infos[n_properties - empty_families]);
				--i;
			}
		}

		std::vector<char const *> extensions {};
		deviceExtensions(selected_device, extensions);
		vk::PhysicalDeviceFeatures features {};
		deviceFeatures(selected_device, features);

		vk::DeviceCreateInfo const create_info { {},
			n_properties - empty_families, queue_infos.data(), 0U, nullptr,
			static_cast<std::uint32_t>(extensions.size()), extensions.data(), &features };
		vk::Result result { selected_device.createDevice(
			&create_info, nullptr, &device, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "createDevice"); }

		device.getQueue(compute.family, compute_index, &compute.queue, dispatch);
		device.getQueue(transfer.family, transfer_index, &transfer.queue, dispatch);
		device.getQueue(present.family, present_index, &present.queue, dispatch);

		auto const pfnDeviceProc { reinterpret_cast<PFN_vkGetDeviceProcAddr>(
			instance.getProcAddr("vkGetDeviceProcAddr", dispatch)) };
		dispatch.init(instance, dispatch.vkGetInstanceProcAddr, device, pfnDeviceProc);
	}
	/// <summary>
	/// Destroys the currently created vulkan logic device.
	/// </summary>
	void Render::destroyDevice() noexcept
	{
		device.destroy(nullptr, dispatch);
	}
	/// <summary>
	/// Builds the entire render framework on the selected device.
	/// Throws any error that might occur.
	/// </summary>
	void Render::createFramework()
	{
		vk::Extent2D base_extent { 0U };
		glfwGetFramebufferSize(core_nucleus.ui.window,
			reinterpret_cast<int *>(&base_extent.width), reinterpret_cast<int *>(&base_extent.width));

		vk::PhysicalDevice & physical_device = physical_devices[device_index].first;

		framework = new RayTracer(dispatch, instance, physical_device, device,
			surface, base_extent, compute.family, transfer.family, present.family,
			static_cast<ThreadPool &>(core_nucleus),
			core_nucleus.display_settings.width, core_nucleus.display_settings.height,
			core_nucleus.environment.guard, core_nucleus.environment.scene);

		createFence(vk::FenceCreateFlagBits::eSignaled, main_fence);
		createSemaphore({}, acquisition_semaphore);
	}
	/// <summary>
	/// Destroys the framework created on the selected device. Must be used
	/// before re-selecting a new device.
	/// </summary>
	void Render::destroyFramework() noexcept
	{
		destroySemaphore(acquisition_semaphore);
		destroyFence(main_fence);
		delete framework;
	}
	/// <summary>
	/// Builds the entire render synchronisation.
	/// Throws any error that might occur.
	/// </summary>
	void Render::setUpDispatch()
	{
		std::size_t n_jobs { core_nucleus.display_settings.anti_aliasing };
		if(n_jobs == 0) { ++n_jobs; }
		n_jobs += 2U;
		dispatch_jobs.resize(n_jobs);
		for(std::size_t j_idx { 0U }; j_idx < n_jobs; ++j_idx)
		{
			DispatchJobs & job = dispatch_jobs[j_idx];

			createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, compute.family, job.c_pool);
			allocCommandBuffers(job.c_pool, vk::CommandBufferLevel::ePrimary, 1U, &job.c_buffer);
			createSemaphore({}, job.c_semaphore);
		}
	}
	/// <summary>
	/// Destroys the synchronisation created on the selected device. Must be
	/// used before re-selecting a new device.
	/// </summary>
	void Render::tearDownDispatch() noexcept
	{
		std::size_t n_jobs { core_nucleus.display_settings.anti_aliasing };
		if(n_jobs == 0) { ++n_jobs; }
		n_jobs += 2U;
		for(std::size_t j_idx { 0U }; j_idx < n_jobs; ++j_idx)
		{
			DispatchJobs & job = dispatch_jobs[j_idx];

			destroySemaphore(job.c_semaphore);
			freeCommandBuffers(job.c_pool, 1U, &job.c_buffer);
			destroyCommandPool(job.c_pool);
		}
	}

	// ------------------------------------------------------------------ //
	// Vulkan device selection.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Queries for all available physical devices. If there are none,
	/// throws an error.
	/// </summary>
	void Render::queryPhysicalDevices()
	{
		std::vector<vk::PhysicalDevice> devices {};
		std::uint32_t n_devices { 0U };
		{
			vk::Result result { vk::Result::eSuccess };
			do
			{
				result = instance.enumeratePhysicalDevices(
					&n_devices, nullptr, dispatch);
				devices.resize(n_devices);
				result = instance.enumeratePhysicalDevices(
					&n_devices, devices.data(), dispatch);
			}
			while(result == vk::Result::eIncomplete);
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "enumeratePhysicalDevices"); }
		}
		physical_devices.resize(n_devices);
		for(std::uint32_t i { 0U }; i < n_devices; ++i)
		{
			physical_devices[i].first = devices[i];
			physical_devices[i].second = true;
		}
	}
	/// <summary>
	/// Checks all devices if they're fit the application requirements.
	/// If none are fit, throws an error.
	/// </summary>
	void Render::checkPhysicalDevices()
	{
		std::uint32_t n_devices { static_cast<std::uint32_t>(physical_devices.size()) };
		for(std::uint32_t i { 0U }; i < n_devices; ++i)
		{
			if(!isDeviceFit(physical_devices[i].first))
			{ physical_devices[i].second = false; }
		}
	}
	/// <summary>
	/// Selects the device with the name described in the settings. If
	/// there is no device in settings or it doesn't exist selects the first
	/// found and fills in the name with the new one.
	/// </summary>
	void Render::selectPhysicalDevice() noexcept
	{
		std::uint32_t n_devices { static_cast<std::uint32_t>(physical_devices.size()) };
		std::string & device_name { core_nucleus.display_settings.device_name };
		if(!device_name.empty())
		{
			for(std::uint32_t i { 0U }; i < n_devices; ++i)
			{
				vk::PhysicalDeviceProperties properties {};
				physical_devices[i].first.getProperties(&properties, dispatch);

				bool const isPreSelected { device_name == properties.deviceName };
				if(physical_devices[i].second && isPreSelected)
				{ device_index = i; return; }
			}
		}
		for(std::uint32_t i { 0U }; i < n_devices; ++i)
		{
			if(physical_devices[i].second)
			{
				device_index = i;
				vk::PhysicalDeviceProperties properties {};
				physical_devices[i].first.getProperties(&properties, dispatch);
				device_name = properties.deviceName;
			}
		}
	}

	// ------------------------------------------------------------------ //
	// Helpers.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Finds a queue family with the given type flags. Returns if one is found
	/// and updates the given family index with found queue index.
	/// </summary>
	bool Render::typeFamily(std::vector<vk::QueueFamilyProperties> const & families,
		vk::QueueFlags const & type, std::uint32_t & family,
		bool const type_only) const noexcept
	{
		std::uint32_t const n_families { static_cast<std::uint32_t>(families.size()) };
		if(type_only)
		{
			for(std::uint32_t i { 0U }; i < n_families; ++i)
			{
				if(families[i].queueFlags == type)
				{ family = i; return true; }
			}
		}
		for(std::uint32_t i { 0U }; i < n_families; ++i)
		{
			if(families[i].queueFlags & type)
			{ family = i; return true; }
		}
		return false;
	}
	/// <summary>
	/// Finds a present family. Returns if one is found and updates the given
	/// family index with found queue index.
	/// </summary>
	bool Render::presentFamily(std::vector<vk::QueueFamilyProperties> const & families,
		vk::PhysicalDevice const & physical_device, std::uint32_t & family) const
	{
		std::uint32_t const n_families { static_cast<std::uint32_t>(families.size()) };
		for(std::uint32_t i { 0U }; i < n_families; ++i)
		{
			vk::Bool32 supported { 0U };
			vk::Result result = physical_device.getSurfaceSupportKHR(
				i, surface, &supported, dispatch);
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "getSurfaceSupportKHR"); }

			if(supported)
			{ family = i; return true; }
		}
		return false;
	}
	/// <summary>
	/// Fills the required extensions and can check if all required
	/// extensions are available.
	/// </summary>
	bool Render::deviceExtensions(vk::PhysicalDevice const & physical_device,
		std::vector<char const *> & required, bool const check) const
	{
		required.clear();
		required.emplace_back("VK_KHR_swapchain");
		if(check)
		{
			std::vector<vk::ExtensionProperties> extensions {};
			std::uint32_t n_extensions { 0U };
			{
				vk::Result result { vk::Result::eSuccess };
				do
				{
					result = physical_device.enumerateDeviceExtensionProperties(
						nullptr, &n_extensions, nullptr, dispatch);
					extensions.resize(n_extensions);
					result = physical_device.enumerateDeviceExtensionProperties(
						nullptr, &n_extensions, extensions.data(), dispatch);
				}
				while(result == vk::Result::eIncomplete);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "enumeratePhysicalDevices"); }
			}
			for(auto const & requirement : required)
			{
				bool found { false };
				for(std::uint32_t i { 0U }; i < n_extensions; ++i)
				{
					if(!std::strcmp(extensions[i].extensionName, requirement))
					{ found = true; break; }
				}
				if(!found) { return false; }
			}
		}
		return true;
	}
	/// <summary>
	/// Fills the required features and can check if all required extensions
	/// are available.
	/// </summary>
	bool Render::deviceFeatures(vk::PhysicalDevice const & physical_device,
		vk::PhysicalDeviceFeatures & required, bool const check) const
	{
		required = {};
		if(check)
		{
			vk::PhysicalDeviceFeatures features;
			physical_device.getFeatures(&features, dispatch);
		}
		return true;
	}
	/// <summary>
	/// Checks if the given device if fit for the application needs.
	/// </summary>
	bool Render::isDeviceFit(vk::PhysicalDevice const & physical_device) const noexcept
	{
		// Extension and feature support.
		std::vector<char const *> extensions {};
		if(!deviceExtensions(physical_device, extensions, true))
		{ return false; };
		vk::PhysicalDeviceFeatures features {};
		if(!deviceFeatures(physical_device, features, true))
		{ return false; };
		// Queue support.
		std::vector<vk::QueueFamilyProperties> properties {};
		std::uint32_t n_properties { 0U };
		physical_device.getQueueFamilyProperties(&n_properties, nullptr, dispatch);
		properties.resize(n_properties);
		physical_device.getQueueFamilyProperties(&n_properties, properties.data(), dispatch);
		std::uint32_t compute_index { 0U }, transfer_index { 0U }, present_index { 0U };
		if(!typeFamily(properties, vk::QueueFlagBits::eCompute, compute_index))
		{ return false; };
		if(!typeFamily(properties, vk::QueueFlagBits::eTransfer, transfer_index))
		{ return false; };
		if(!presentFamily(properties, physical_device, present_index))
		{ return false; };
		// Surface support.
		std::uint32_t n_formats { 0U }, n_modes { 0U };
		physical_device.getSurfaceFormatsKHR(surface, &n_formats, nullptr, dispatch);
		if(n_formats == 0U) { return false; }
		physical_device.getSurfacePresentModesKHR(surface, &n_modes, nullptr, dispatch);
		if(n_modes == 0U) { return false; }
		return true;
	}
	/// <summary>
	/// Adds new queue to create info structure at the respective family and
	/// updates priorities with all queues having the same priority.
	/// </summary>
	void Render::addQueueToCreateInfo(std::uint32_t const & family,
		std::vector<vk::DeviceQueueCreateInfo> & queue_infos,
		std::vector<std::vector<float>> & priorities,
		std::uint32_t & queue_index) const noexcept
	{
		queue_index = queue_infos[family].queueCount;
		std::uint32_t const n_queues = ++queue_infos[family].queueCount;
		priorities[family].resize(n_queues);
		for(std::uint32_t i { 0U }; i < n_queues; ++i)
		{ priorities[family][i] = 1.0f / static_cast<float>(n_queues); }
	}
	/// <summary>
	/// Debug callback to be assigned for validation layers.
	/// </summary>
	VkBool32 Render::vulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity_flag,
		VkDebugUtilsMessageTypeFlagsEXT type_flags,
		VkDebugUtilsMessengerCallbackDataEXT const * data,
		[[maybe_unused]] void * user_data)
	{
		auto const severity { static_cast
			<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity_flag) };
		auto const type { static_cast
			<vk::DebugUtilsMessageTypeFlagsEXT>(type_flags) };
		// Show message.
		std::cout << "Vulkan layer message ["
			<< vk::to_string(severity) << ", " << vk::to_string(type)
			<< "]:\n" << data->pMessage << "\n";
		// Throw exception if warning or error.
		if(severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{ throw std::exception("Layer error or warning."); }
		// Return must be always VK_FALSE.
		return VK_FALSE;
	}
}
