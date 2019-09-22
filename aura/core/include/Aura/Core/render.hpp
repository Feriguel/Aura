// ========================================================================== //
// File : render.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER
#define AURACORE_RENDER
// Internal includes.
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <array>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)

namespace Aura::Core
{
	// Engine core module.
	class Nucleus;
	// Ray tracing framework.
	class RayTracer;
	/// <summary>
	/// Englobes the queue and its respective family index.
	/// </summary>
	struct RenderQueue
	{
		std::uint32_t family { std::numeric_limits<std::uint32_t>::max() };
		vk::Queue queue {};
	};
	/// <summary>
	/// Work type buffer indices and buffer count.
	/// </summary>
	struct WorkTypes
	{
		static constexpr std::uint32_t n { 4U };
		static constexpr std::uint32_t ray_gen { 0U };
		static constexpr std::uint32_t intersect { 1U };
		static constexpr std::uint32_t colour { 2U };
		static constexpr std::uint32_t scatter { 3U };
	};
	/// <summary>
	/// Joins all necessary elements required for a single thread to build dispatch.
	/// </summary>
	struct SampleDispatch
	{
		// Thread compute command pool.
		vk::CommandPool c_pool {};
		// Thread compute command buffers.
		std::array<vk::CommandBuffer, WorkTypes::n> c_buffers {};
		// Thread compute command semaphores.
		std::array<vk::Semaphore, WorkTypes::n> c_semaphores {};
	};
	/// <summary>
	/// Program render and render cycle. Uses a vulkan compute ray tracer as a
	/// render. Takes care of vulkan instance, debug and surface creation, as
	/// well as, device selection, commands and sync.
	/// </summary>
	class Render
	{
		// Nucleus handler.
		Nucleus & core_nucleus;
		// Vulkan dynamic dispatcher.
		vk::DispatchLoaderDynamic dispatch;
		// Vulkan instance handle.
		vk::Instance instance;
		// Vulkan debug callback handle;
		vk::DebugUtilsMessengerEXT debug_messeger;
		// Vulkan surface handle.
		vk::SurfaceKHR surface;
		// Vulkan physical devices handles and respective availability.
		std::vector<std::pair<vk::PhysicalDevice, bool>> physical_devices;
		// Selected physical device.
		std::uint32_t device_index;
		// Vulkan logic device handle.
		vk::Device device;
		// Vulkan device handles.
		RenderQueue compute, transfer, present;
		// Vulkan compute ray tracing framework.
		RayTracer * framework;

		// Permanent shader flags.
		std::array<vk::PipelineStageFlags, 4> const stage_flags;
		// Dispatch structures for each thread.
		std::vector<SampleDispatch> sample_dispatches;
		// Render thread compute command pool.
		vk::CommandPool c_pool;
		// Main tasks compute command buffer.
		std::array<vk::CommandBuffer, 3U> c_buffers;
		// Main tasks semaphores.
		std::array<vk::Semaphore, 3U> main_semaphores;
		// Render fence.
		vk::Fence main_fence;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the base core and starts rendering.
		/// </summary>
		explicit Render(Nucleus & nucleus);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~Render() noexcept;

		// ------------------------------------------------------------------ //
		// Render control.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Renders and presents new frame.
		/// </summary>
		bool renderFrame();
		/// <summary>
		/// Checks for any updates in the environment and clones the new states
		/// to the GPU. Given jobs array must be empty. Checks if all jobs are valid.
		/// </summary>
		void updateEnvironment(std::uint32_t const & frame_idx) const;
		/// <summary>
		/// Submits a layout change to general for the frame at the given index.
		/// This task is submitted without any kind of fence.
		/// </summary>
		void updateImageLayoutToGeneral(std::uint32_t const & frame_idx) const;
		/// <summary>
		/// Renders all image samples by dispatching a thread for each sample
		/// command record. This will generate all rays, process them and, at
		/// the end, schedule a post-process.
		/// </summary>
		void renderImage() const;
		/// <summary>
		/// Submits a layout change to present for the frame at the given index.
		/// This task is submitted with the main fence, which should be waited.
		/// </summary>
		void updateImageLayoutToPresent(std::uint32_t const & frame_idx) const;
		/// <summary
		/// Waits for the main fence until all its tasks are finished.
		/// </summary>
		void waitForMainFence() const;
		/// <summary
		/// Waits for the device to complete all it's given tasks, must be used
		/// before destroying any of the tear-down functions.
		/// </summary>
		void waitIdle() const noexcept;
		/// <summary>
		/// Randomizes a value limited to a circle with 1 unit radius.
		/// </summary>
		float randomInCircle() const;

		// ------------------------------------------------------------------ //
		// Command creation.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Starts the command buffer record operation.
		/// </summary>
		void beginRecord(vk::CommandBufferUsageFlags const & flags,
			vk::CommandBufferInheritanceInfo const & inheritance_info,
			vk::CommandBuffer const & command) const;
		/// <summary>
		/// Ends the command buffer record operation.
		/// </summary>
		void endRecord(vk::CommandBuffer const & command) const;
		/// <summary>
		/// Builds a sample submission, and must only be called by a pool thread.
		/// Returns either a list of submissions or an empty list if the previous
		/// submission is still on going. In this case the record should enqueued
		/// again.
		/// </summary>
		std::array<vk::SubmitInfo, WorkTypes::n> const recordSample(bool const is_random, std::uint32_t const sample_idx) const;
		/// <summary>
		/// Records the ray generation command buffer and the builds the submit
		/// info according to the to conditions given. Upon completion signals
		/// the thread corresponding work semaphore.
		/// </summary>
		void recordRayGenerationStage(SampleDispatch const & sample, std::uint32_t const n_wait,
			vk::Semaphore const * p_wait, bool const & is_random, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records the intersect stage command buffer and the builds the submit
		/// info. Upon completion signals the thread corresponding work semaphore.
		/// </summary>
		void recordIntersectStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records the colour stage command buffer and the builds the submit
		/// info. Upon completion signals the thread corresponding work semaphore.
		/// </summary>
		void recordColourStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records the scatter stage command buffer and the builds the submit
		/// info. Upon completion signals the thread corresponding work semaphore.
		/// </summary>
		void recordScatterStage(SampleDispatch const & sample, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records the post-process command buffer and the builds the submit
		/// info. This task signal semaphore is the second main semaphore.
		/// </summary>
		void recordPostProcessStage(vk::CommandBuffer const & buffer,
			std::uint32_t const n_wait, vk::Semaphore const * p_wait, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records layout transition of the frame at the given index to general.
		/// </summary>
		void recordGeneralTransition(vk::CommandBuffer const & buffer,
			std::uint32_t const frame_idx, vk::SubmitInfo & submit) const;
		/// <summary>
		/// Records layout transition of the frame at the given index to present.
		/// </summary>
		void recordPresentTransition(vk::CommandBuffer const & buffer,
			std::uint32_t const frame_idx, vk::SubmitInfo & submit) const;

		// ------------------------------------------------------------------ //
		// Vulkan set-up and tear-down related.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Sets up vulkan by initialising the dynamic dispatcher, instance and
		/// debug messaging (if debug).
		/// It doesn't verify if extensions or layers are supported, as the render
		/// wouldn't work without them.
		/// Throws any error that might occur.
		/// </summary>
		void initVulkan();
		/// <summary>
		/// Tears down vulkan by destroying both created instance and debug
		/// messaging.
		/// </summary>
		void terminateVulkan() noexcept;
		/// <summary>
		/// Creates a compute command pool with the given flags.
		/// Throws any error that might occur.
		/// </summary>
		void createCommandPool(vk::CommandPoolCreateFlags const & flags,
			std::uint32_t const & family, vk::CommandPool & pool) const;
		/// <summary>
		/// Destroys command pool.
		/// </summary>
		void destroyCommandPool(vk::CommandPool & pool) noexcept;
		/// <summary>
		/// Allocates N buffers at the target data pointer on the given command
		/// pool with intended level.
		/// Throws any error that might occur.
		/// </summary>
		void allocCommandBuffers(
			vk::CommandPool const & pool, vk::CommandBufferLevel const & level,
			std::uint32_t const & n_buffers, vk::CommandBuffer * const p_buffers) const;
		/// <summary>
		/// Frees N buffers at the target data pointer on the given command pool.
		/// </summary>
		void freeCommandBuffers(vk::CommandPool const & pool,
			std::uint32_t const & n_buffers, vk::CommandBuffer * const p_buffers) noexcept;
		/// <summary>
		/// Creates a semaphore.
		/// Throws any error that might occur.
		/// </summary>
		void createSemaphore(vk::SemaphoreCreateFlags const & flags, vk::Semaphore & semaphore) const;
		/// <summary>
		/// Destroys a semaphore.
		/// </summary>
		void destroySemaphore(vk::Semaphore & semaphore) noexcept;
		/// <summary>
		/// Create a fence.
		/// Throws any error that might occur.
		/// </summary>
		void createFence(vk::FenceCreateFlags const & flags, vk::Fence & fence) const;
		/// <summary>
		/// Destroys a fence.
		/// </summary>
		void destroyFence(vk::Fence & fence) noexcept;
		public:
		/// <summary>
		/// Creates the vulkan surface for the current window.
		/// Throws any error that might occur.
		/// </summary>
		void createSurface();
		/// <summary>
		/// Destroys the currently created vulkan surface.
		/// </summary>
		void destroySurface() noexcept;
		/// <summary>
		/// Creates the vulkan logic device from the selected physical device
		/// as well as all the queues. Also updates the dispatcher for the new
		/// device.
		/// </summary>
		void createDevice();
		/// <summary>
		/// Destroys the currently created vulkan logic device.
		/// </summary>
		void destroyDevice() noexcept;
		/// <summary>
		/// Builds the entire render framework on the selected device.
		/// Throws any error that might occur.
		/// </summary>
		void createFramework();
		/// <summary>
		/// Destroys the framework created on the selected device. Must be used
		/// before re-selecting a new device.
		/// </summary>
		void destroyFramework() noexcept;
		/// <summary>
		/// Builds the entire render synchronisation.
		/// Throws any error that might occur.
		/// </summary>
		void setUpDispatch();
		/// <summary>
		/// Destroys the synchronisation created on the selected device. Must be
		/// used before re-selecting a new device.
		/// </summary>
		void tearDownDispatch() noexcept;

		// ------------------------------------------------------------------ //
		// Vulkan device selection.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Queries for all available physical devices. If there are none,
		/// throws an error.
		/// </summary>
		void queryPhysicalDevices();
		public:
		/// <summary>
		/// Checks all devices if they're fit the application requirements.
		/// If none are fit, throws an error.
		/// </summary>
		void checkPhysicalDevices();
		/// <summary>
		/// Selects the device with the name described in the settings. If
		/// there is no device in settings or it doesn't exist selects the first
		/// found and fills in the name with the new one.
		/// </summary>
		void selectPhysicalDevice() noexcept;

		// ------------------------------------------------------------------ //
		// Vulkan related helpers.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Finds a queue family with the given type flags. Returns if one is found
		/// and updates the given family index with found queue index.
		/// </summary>
		bool typeFamily(std::vector<vk::QueueFamilyProperties> const & families,
			vk::QueueFlags const & type, std::uint32_t & family,
			bool const type_only = false) const noexcept;
		/// <summary>
		/// Finds a present family. Returns if one is found and updates the given
		/// family index with found queue index.
		/// </summary>
		bool presentFamily(std::vector<vk::QueueFamilyProperties> const & families,
			vk::PhysicalDevice const & physical_device, std::uint32_t & family) const;
		/// <summary>
		/// Fills the required extensions and can check if all required
		/// extensions are available.
		/// </summary>
		bool deviceExtensions(vk::PhysicalDevice const & physical_device,
			std::vector<char const *> & required, bool const check = false) const;
		/// <summary>
		/// Fills the required features and can check if all required extensions
		/// are available.
		/// </summary>
		bool deviceFeatures(vk::PhysicalDevice const & physical_device,
			vk::PhysicalDeviceFeatures & required, bool const check = false) const;
		/// <summary>
		/// Checks if the given device if fit for the application needs.
		/// </summary>
		bool isDeviceFit(vk::PhysicalDevice const & physical_device) const noexcept;
		/// <summary>
		/// Adds new queue to create info structure at the respective family and
		/// updates priorities with all queues having the same priority.
		/// </summary>
		void addQueueToCreateInfo(std::uint32_t const & family,
			std::vector<vk::DeviceQueueCreateInfo> & queue_infos,
			std::vector<std::vector<float>> & priorities,
			std::uint32_t & compute_index) const noexcept;
		public:
		/// <summary>
		/// Debug callback to be assigned for validation layers.
		/// </summary>
		static VkBool32 vulkanDebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity_flag,
			VkDebugUtilsMessageTypeFlagsEXT type_flags,
			VkDebugUtilsMessengerCallbackDataEXT const * data,
			void * user_data);

		// ------------------------------------------------------------------ //
		// Obtain information.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Obtains the list of physical devices which are fit for the app.
		/// </summary>
		constexpr std::vector<std::pair<vk::PhysicalDevice, bool>> const & getPhysicalDevices() const noexcept
		{ return physical_devices; }
	};

}

#endif
