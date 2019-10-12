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
#pragma warning(disable : 26812)
#include <glm/vec3.hpp>
#pragma warning(default : 26812)

namespace Aura::Core
{
	// Engine core module.
	class Nucleus;
	// Ray tracing framework.
	class RayTracer;
	// Push random values.
	struct RandomSeed;
	struct RandomPointInCircleAndSeed;
	/// <summary>
	/// Englobes the queue and its respective family index.
	/// </summary>
	struct RenderQueue
	{
		std::uint32_t family { std::numeric_limits<std::uint32_t>::max() };
		vk::Queue queue {};
	};
	/// <summary>
	/// Joins all necessary elements required for a single thread to build each
	/// job dispatch.
	/// </summary>
	struct DispatchJobs
	{
		// Thread compute command pool.
		vk::CommandPool c_pool {};
		// Thread compute command buffers.
		vk::CommandBuffer c_buffer {};
		// Thread compute command semaphores.
		vk::Semaphore c_semaphore {};
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
		std::vector<DispatchJobs> dispatch_jobs;
		// Render fence.
		vk::Fence main_fence;
		// Frame acquisition semaphore.
		vk::Semaphore acquisition_semaphore {};

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
		bool dispatchFrame();
		/// <summary
		/// Waits for the device to complete all it's given tasks, must be used
		/// before destroying any of the tear-down functions.
		/// </summary>
		void waitIdle() const noexcept;
		/// <summary>
		/// Waits for the main fence until all its tasks are finished.
		/// </summary>
		bool waitForMainFence(std::uint64_t timeout) const;
		private:
		/// <summary>
		/// Checks for any updates in the environment and clones the new states
		/// to the GPU. Waits for any required work to finish.
		/// </summary>
		bool updateEnvironment(std::uint32_t const & frame_idx) const;
		/// <summary>
		/// Records and submits all necessary commands to render the image in
		/// the current settings.
		/// </summary>
		void dispatchFrameJobs(std::uint32_t const & frame_idx, bool const & update) const;

		// ------------------------------------------------------------------ //
		// Command recording and submission schedule.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Builds the entire submit info as necessary for queue submission.
		/// </summary>
		void dispatchSubmitInfo(const std::size_t n_submits, std::vector<vk::SubmitInfo> & submits) const;
		/// <summary>
		/// Records the layout transition to geral to a initial submission.
		/// </summary>
		void recordPreProcess(std::uint32_t const frame_idx, bool const update) const;
		/// <summary>
		/// Records a sample sequence in the buffer associated with the sample
		/// index. Each sequence includes a ray-generation and x sets of 
		/// intersect, colour and scatter, in this order, equal to the maximum
		/// depth.
		/// </summary>
		void recordSample(bool const is_random, std::size_t const sample_idx) const;
		/// <summary>
		/// Records both the post-process and the layout transition, in this order,
		/// to the same submission.
		/// </summary>
		void recordPostProcess(std::uint32_t const frame_idx) const;
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
		/// Randomizes a value limited to a circle with 1 unit radius.
		/// </summary>
		glm::vec3 randomInCircle() const;
		/// <summary>
		/// Fills structure with sets of random vectors within a circle.
		/// </summary>
		void fillRandomsWithinCircle(RandomPointInCircleAndSeed & randoms) const;
		/// <summary>
		/// Fills structure with generated values.
		/// </summary>
		void fillRandoms(RandomSeed & randoms) const;

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
