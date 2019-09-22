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
	bool Render::renderFrame()
	{
		std::uint32_t frame_idx { 0U };

		// Try to acquire frame.
		if(!framework->acquireframe(nullptr, main_fence, 0, frame_idx))
		{
			return false;
		}
		// Wait for the frame acquisition.
		waitForMainFence();
		// Enqueue environment update jobs and wait for them to finish.
		updateEnvironment(frame_idx);

		// Update image layout to general to permit render operations.
		updateImageLayoutToGeneral(frame_idx);
		// Render image with the updated environment.
		renderImage();
		// Update image layout to present to permit its display.
		updateImageLayoutToPresent(frame_idx);

		// Set image for display.
		framework->displayFrame(1U, &main_semaphores[2U], frame_idx, present.queue);
		// Wait for image to render.
		waitForMainFence();
		return true;
	}
	/// <summary>
	/// Checks for any updates in the environment and clones the new states
	/// to the GPU. Given jobs array must be empty. Checks if all jobs are valid.
	/// </summary>
	void Render::updateEnvironment(std::uint32_t const & frame_idx) const
	{
		std::vector<std::future<void>> jobs {};

		// Update chain image descriptor set.
		jobs.emplace_back(core_nucleus.enqueue([&] { framework->updateChainImageSet(frame_idx); }));
		// Update ray launcher with camera changes.
		jobs.emplace_back(core_nucleus.enqueue([&]
		{
			framework->updateRayLauncher(
				core_nucleus.display_settings.ray_depth);
		}));

		for(std::size_t i { 0U }; i < jobs.size(); ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
	}
	/// <summary>
	/// Submits a layout change to general for the frame at the given index.
	/// This task is submitted without any kind of fence.
	/// </summary>
	void Render::updateImageLayoutToGeneral(std::uint32_t const & frame_idx) const
	{
		vk::SubmitInfo submit {};
		recordGeneralTransition(c_buffers[0U], frame_idx, submit);
		compute.queue.submit(1U, &submit, nullptr, dispatch);
	}
	/// <summary>
	/// Renders all image samples by dispatching a thread for each sample
	/// command record. This will generate all rays, process them and, at
	/// the end, schedule a post-process.
	/// </summary>
	void Render::renderImage() const
	{
		// Check is n_samples is 0
		std::uint32_t n_samples { core_nucleus.display_settings.anti_aliasing };
		bool is_random { n_samples != 0U };
		if(!is_random) { ++n_samples; }

		// Resize to fit all submits.
		// std::uint32_t n_submits { n_samples * WorkTypes::n + 1 };
		std::uint32_t n_submits { 2U };
		std::vector<vk::SubmitInfo> submits {};
		submits.resize(n_submits);

		/*
		// Schedule all sample command recording jobs and fill submits with the results.
		std::vector<std::future<std::array<vk::SubmitInfo, WorkTypes::n>>> jobs {};
		jobs.resize(n_samples);
		for(std::uint32_t i { 0U }; i < n_samples; ++i)
		{
			jobs[i] = core_nucleus.enqueue([=] { return recordSample(is_random, i); });
		}
		// Record post-process to last job.
		recordPostProcessStage(c_buffers[1U], 1U,
			&sample_dispatches[n_samples - 1].c_semaphores[WorkTypes::scatter], submits[n_submits - 1]);
		// Fill all submit infos with thread results.
		for(std::size_t i { 0U }; i < static_cast<std::size_t>(n_samples); ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
			std::array<vk::SubmitInfo, WorkTypes::n> result { std::move(jobs[i].get()) };
			for(std::size_t j { 0U }; j < static_cast<std::size_t>(WorkTypes::n); ++j)
			{
				submits[i * static_cast<std::size_t>(WorkTypes::n) + j] = result[j];
			}
		}
		*/

		RandomOffset randoms {};
		if(is_random)
		{
			randoms.s = randomInCircle();
			randoms.t = randomInCircle();
		}
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, c_buffers[1U]);
		framework->recordRayGen(randoms, c_buffers[1U]);
		for(std::uint32_t i { 0U }; i < n_samples; ++i)
		{
			framework->recordIntersect(c_buffers[1U]);
			framework->recordIntersect(c_buffers[1U]);
			framework->recordIntersect(c_buffers[1U]);
		}
		framework->recordPostProcess(c_buffers[1U]);
		endRecord(c_buffers[1U]);
		submits[0].setWaitSemaphoreCount(1U).setPWaitSemaphores(&main_semaphores[0U]);
		submits[0].setPWaitDstStageMask(&stage_flags[1U]);
		submits[0].setCommandBufferCount(1U).setPCommandBuffers(&c_buffers[1U]);
		submits[0].setSignalSemaphoreCount(1U).setPSignalSemaphores(&main_semaphores[1U]);

		// Submit all jobs.
		compute.queue.submit(n_submits, submits.data(), nullptr, dispatch);
	}
	/// <summary>
	/// Submits a layout change to present for the frame at the given index.
	/// This task is submitted with the main fence, which should be waited.
	/// </summary>
	void Render::updateImageLayoutToPresent(std::uint32_t const & frame_idx) const
	{
		vk::SubmitInfo submit {};
		recordPresentTransition(c_buffers[2U], frame_idx, submit);
		compute.queue.submit(1U, &submit, main_fence, dispatch);
	}
	/// <summary
	/// Waits for the main fence until all its tasks are finished.
	/// </summary>
	void Render::waitForMainFence() const
	{
		vk::Result result { vk::Result::eSuccess };
		result = device.waitForFences(1U, &main_fence, static_cast<vk::Bool32>(false),
			std::numeric_limits<std::uint64_t>::max(), dispatch);
		while(result != vk::Result::eSuccess)
		{
			result = device.waitForFences(1U, &main_fence, static_cast<vk::Bool32>(false),
				std::numeric_limits<std::uint64_t>::max(), dispatch);
			if(!(result == vk::Result::eSuccess || result == vk::Result::eTimeout))
			{ vk::throwResultException(result, "Present layout wait."); }
		}
		device.resetFences(1U, &main_fence, dispatch);
	}
	/// <summary>
	/// Waits for the device to complete all it's given tasks, must be used
	/// before destroying any of the tear-down functions.
	/// </summary>
	void Render::waitIdle() const noexcept
	{
		device.waitIdle(dispatch);
	}
	/// <summary>
	/// Randomizes a value limited to a circle with 1 unit radius.
	/// </summary>
	float Render::randomInCircle() const
	{
		float rand = 0;
		do
		{
			rand = core_nucleus.gen();
		}
		while(glm::dot(rand, rand) >= 1.0);
		return rand;
	}

	// ------------------------------------------------------------------ //
	// Commands.
	// ------------------------------------------------------------------ //
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
	/// Builds a sample submission, and must only be called by a pool thread.
	/// Returns either a list of submissions or an empty list if the previous
	/// submission is still on going. In this case the record should enqueued
	/// again.
	/// </summary>
	std::array<vk::SubmitInfo, WorkTypes::n> const Render::recordSample(
		bool const is_random, std::uint32_t const sample_idx) const
	{
		SampleDispatch const & sample { sample_dispatches[sample_idx] };
		std::array<vk::SubmitInfo, WorkTypes::n> submits {};

		// Record commands for each of the shader stages.
		if(sample_idx == 0)
		{
			recordRayGenerationStage(sample, 1U, &main_semaphores[0U],
				is_random, submits[WorkTypes::ray_gen]);
		}
		else
		{
			recordRayGenerationStage(sample, 1U, &sample_dispatches[sample_idx - 1].c_semaphores[WorkTypes::scatter],
				is_random, submits[WorkTypes::ray_gen]);
		}
		recordIntersectStage(sample, submits[WorkTypes::intersect]);
		recordColourStage(sample, submits[WorkTypes::colour]);
		recordScatterStage(sample, submits[WorkTypes::scatter]);
		return submits;
	}
	/// <summary>
	/// Records the ray generation command buffer and the builds the submit
	/// info according to the to conditions given. Upon completion signals
	/// the thread corresponding work semaphore.
	/// </summary>
	void Render::recordRayGenerationStage(SampleDispatch const & sample, std::uint32_t const n_wait,
		vk::Semaphore const * p_wait, bool const & is_random, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, sample.c_buffers[WorkTypes::ray_gen]);
		RandomOffset randoms {};
		if(is_random)
		{
			randoms.s = randomInCircle();
			randoms.t = randomInCircle();
		}
		framework->recordRayGen(randoms, sample.c_buffers[WorkTypes::ray_gen]);
		endRecord(sample.c_buffers[WorkTypes::ray_gen]);

		submit.setWaitSemaphoreCount(n_wait).setPWaitSemaphores(p_wait);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&sample.c_buffers[WorkTypes::ray_gen]);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&sample.c_semaphores[WorkTypes::ray_gen]);
	}
	/// <summary>
	/// Records the intersect stage command buffer and the builds the submit
	/// info. Upon completion signals the thread corresponding work semaphore.
	/// </summary>
	void Render::recordIntersectStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, sample.c_buffers[WorkTypes::intersect]);
		framework->recordIntersect(sample.c_buffers[WorkTypes::intersect]);
		endRecord(sample.c_buffers[WorkTypes::intersect]);

		submit.setWaitSemaphoreCount(1U).setPWaitSemaphores(&sample.c_semaphores[WorkTypes::ray_gen]);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&sample.c_buffers[WorkTypes::intersect]);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&sample.c_semaphores[WorkTypes::intersect]);
	}
	/// <summary>
	/// Records the colour stage command buffer and the builds the submit
	/// info. Upon completion signals the thread corresponding work semaphore.
	/// </summary>
	void Render::recordColourStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, sample.c_buffers[WorkTypes::colour]);
		framework->recordIntersect(sample.c_buffers[WorkTypes::colour]);
		endRecord(sample.c_buffers[WorkTypes::colour]);

		submit.setWaitSemaphoreCount(1U).setPWaitSemaphores(&sample.c_semaphores[WorkTypes::intersect]);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&sample.c_buffers[WorkTypes::colour]);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&sample.c_semaphores[WorkTypes::colour]);
	}
	/// <summary>
	/// Records the scatter stage command buffer and the builds the submit
	/// info. Upon completion signals the thread corresponding work semaphore.
	/// </summary>
	void Render::recordScatterStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, sample.c_buffers[WorkTypes::scatter]);
		framework->recordIntersect(sample.c_buffers[WorkTypes::scatter]);
		endRecord(sample.c_buffers[WorkTypes::scatter]);

		submit.setWaitSemaphoreCount(1U).setPWaitSemaphores(&sample.c_semaphores[WorkTypes::colour]);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&sample.c_buffers[WorkTypes::scatter]);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&sample.c_semaphores[WorkTypes::scatter]);
	}
	/// <summary>
	/// Records the post-process command buffer and the builds the submit
	/// info. This task signal semaphore is the second main semaphore.
	/// </summary>
	void Render::recordPostProcessStage(vk::CommandBuffer const & buffer,
		std::uint32_t const n_wait, vk::Semaphore const * p_wait, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, buffer);
		framework->recordIntersect(buffer);
		endRecord(buffer);

		submit.setWaitSemaphoreCount(n_wait).setPWaitSemaphores(p_wait);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&buffer);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&main_semaphores[1U]);
	}
	/// <summary>
	/// Records layout transition of the frame at the given index to general.
	/// </summary>
	void Render::recordGeneralTransition(vk::CommandBuffer const & buffer,
		std::uint32_t const frame_idx, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, buffer);
		framework->recordChainImageLayoutTransition(frame_idx,
			{}, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
			compute.family, compute.family, stage_flags[0], stage_flags[1], buffer);
		endRecord(buffer);

		submit.setWaitSemaphoreCount(0U).setPWaitSemaphores(nullptr);
		submit.setPWaitDstStageMask(&stage_flags[0U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&buffer);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&main_semaphores[0U]);
	}
	/// <summary>
	/// Records layout transition of the frame at the given index to present.
	/// </summary>
	void Render::recordPresentTransition(vk::CommandBuffer const & buffer,
		std::uint32_t const frame_idx, vk::SubmitInfo & submit) const
	{
		beginRecord(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {}, buffer);
		framework->recordChainImageLayoutTransition(frame_idx,
			vk::AccessFlagBits::eShaderWrite, {}, vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
			compute.family, present.family, stage_flags[1], stage_flags[3], buffer);
		endRecord(buffer);

		submit.setWaitSemaphoreCount(1U).setPWaitSemaphores(&main_semaphores[1U]);
		submit.setPWaitDstStageMask(&stage_flags[1U]);
		submit.setCommandBufferCount(1U).setPCommandBuffers(&buffer);
		submit.setSignalSemaphoreCount(1U).setPSignalSemaphores(&main_semaphores[2U]);
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

		createSemaphore({}, main_semaphores[0U]);
		createSemaphore({}, main_semaphores[1U]);
		createSemaphore({}, main_semaphores[2U]);
		createFence({}, main_fence);
		createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, compute.family, c_pool);
		allocCommandBuffers(c_pool, vk::CommandBufferLevel::ePrimary, 3U, c_buffers.data());
	}
	/// <summary>
	/// Destroys the framework created on the selected device. Must be used
	/// before re-selecting a new device.
	/// </summary>
	void Render::destroyFramework() noexcept
	{
		freeCommandBuffers(c_pool, 3U, c_buffers.data());
		destroyCommandPool(c_pool);
		destroyFence(main_fence);
		destroySemaphore(main_semaphores[2U]);
		destroySemaphore(main_semaphores[1U]);
		destroySemaphore(main_semaphores[0U]);
		delete framework;
	}
	/// <summary>
	/// Builds the entire render synchronisation.
	/// Throws any error that might occur.
	/// </summary>
	void Render::setUpDispatch()
	{
		std::size_t n_samples { core_nucleus.display_settings.anti_aliasing };
		if(n_samples == 0) { ++n_samples; }
		sample_dispatches.resize(n_samples);
		for(std::size_t s_idx { 0U }; s_idx < n_samples; ++s_idx)
		{
			SampleDispatch & sample = sample_dispatches[s_idx];
			// Create compute command pool and allocate buffers.
			createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, compute.family, sample.c_pool);
			allocCommandBuffers(sample.c_pool, vk::CommandBufferLevel::ePrimary,
				WorkTypes::n, sample.c_buffers.data());
			// Create compute semaphores and fence.
			for(std::uint32_t i { 0U }; i < WorkTypes::n; ++i)
			{
				createSemaphore({}, sample.c_semaphores[i]);
			}
		}
	}
	/// <summary>
	/// Destroys the synchronisation created on the selected device. Must be
	/// used before re-selecting a new device.
	/// </summary>
	void Render::tearDownDispatch() noexcept
	{
		std::size_t n_samples { core_nucleus.display_settings.anti_aliasing };
		if(n_samples == 0) { ++n_samples; }
		for(std::size_t s_idx { 0U }; s_idx < n_samples; ++s_idx)
		{
			SampleDispatch & sample = sample_dispatches[s_idx];
			// Destroy compute fence and semaphores.
			for(std::uint32_t i { 0U }; i < WorkTypes::n; ++i)
			{
				destroySemaphore(sample.c_semaphores[i]);
			}
			// Free compute command buffers and destroy pool.
			freeCommandBuffers(sample.c_pool, WorkTypes::n, sample.c_buffers.data());
			destroyCommandPool(sample.c_pool);
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
