// ========================================================================== //
// File : ray_tracer.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RAY_TRACER
#define AURACORE_RAY_TRACER
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
#include <Aura/Core/Render/structures.hpp>
#include "swapchain.hpp"
// Standard includes.
#include <cstdint>
#include <shared_mutex>
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
	// Program's thread pool.
	class ThreadPool;
	/// <summary>
	/// Structure containing all information over a resource, defined by a
	/// descriptor set layout. Can contain multiple sets.
	/// </summary>
	struct Resource
	{
		// Resource descriptor set layout.
		vk::DescriptorSetLayout set_layout {};
		// List of buffers present in resource.
		std::vector<vk::DescriptorBufferInfo> buffers {};
		// List of buffers present in resource.
		std::vector<vk::Image> images {};
		// Resource image views.
		std::vector<vk::ImageView> image_views {};
		// Resource memory allocations.
		std::vector<vk::DeviceMemory> memories {};
		// Resource descriptor set handles.
		vk::DescriptorSet set {};
	};
	/// <summary>
	/// Structure containing information over a single pipeline.
	/// </summary>
	struct Pipeline
	{
		// Pipeline layout.
		vk::PipelineLayout layout {};
		// Pipeline handle.
		vk::Pipeline pipeline {};
	};
	/// <summary>
	/// Composes the entirety of the program's resources and pipelines and
	/// displays record operations for all the pipelines.
	/// </summary>
	class RayTracer : public VulkanSwapchain
	{
		// RayGen work group sizes.
		static constexpr std::uint32_t const group_size[3U] = { 8U, 8U, 1U };
		// Shader folder location
		static constexpr char const * shader_folder = "../aura/core/shaders/";
		// Scene access shared guard.
		std::shared_mutex & scene_guard;
		// Current scene
		Scene * const & scene;
		// Image width in pixels.
		std::uint32_t const & width;
		// Image height in pixels.
		std::uint32_t const & height;
		// Compute family index.
		std::uint32_t const compute_family;
		// Transfer family index.
		std::uint32_t const transfer_family;
		// Vulkan descriptor pool.
		vk::DescriptorPool pool;
		// Ray launcher.
		Resource ray_launcher;
		// Rays and hits.
		Resource rays_and_hits;
		// Pixels state.
		Resource pixels_state;
		// Ray Generation pipeline.
		Pipeline gen;
		// Ray Generation pipeline.
		Pipeline intersect;
		// Ray Generation pipeline.
		Pipeline colour;
		// Ray Generation pipeline.
		Pipeline scatter;
		// Ray Generation pipeline.
		Pipeline post_process;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the base core and starts rendering. Thread pool is used in
		/// framework set-up speed-up.
		/// </summary>
		explicit RayTracer(
			vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
			vk::PhysicalDevice const & physical_device, vk::Device const & device,
			vk::SurfaceKHR const & surface, vk::Extent2D const & chain_base_extent,
			std::uint32_t const compute_family, std::uint32_t const transfer_family,
			std::uint32_t const present_family, ThreadPool & thread_pool,
			std::uint32_t const & width, std::uint32_t const & height,
			std::shared_mutex & scene_guard, Scene * const & scene);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~RayTracer() noexcept;

		// ------------------------------------------------------------------ //
		// Recordings.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Records a ray-generation operation.
		/// </summary>
		void recordRayGen(RandomOffset & push, vk::CommandBuffer const & command) const;
		/// <summary>
		/// Records a intersect operation.
		/// </summary>
		void recordIntersect(vk::CommandBuffer const & command) const;
		/// <summary>
		/// Records a absorption and colouring operation.
		/// </summary>
		void recordColour(vk::CommandBuffer const & command) const;
		/// <summary>
		/// Records a scatter operation.
		/// </summary>
		void recordScatter(vk::CommandBuffer const & command) const;
		/// <summary>
		/// Records a post-processing operation.
		/// </summary>
		void recordPostProcess(vk::CommandBuffer const & command) const;

		// ------------------------------------------------------------------ //
		// Resource updates.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Updates the ray launcher using the camera in the scene.
		/// </summary>
		void updateRayLauncher(std::uint32_t const ray_depth);

		// ------------------------------------------------------------------ //
		// Resources.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Sets up the descriptor pool and bulk sets-up all the resources needed.
		/// </summary>
		void setUpResources(ThreadPool & thread_pool);
		/// <summary>
		/// Bulk tears-down all set-up resources, as well as, the descriptor pool.
		/// </summary>
		void tearDownResources();
		/// <summary>
		/// Allocates all descriptor sets in one go.
		/// </summary>
		void allocateAllDescriptorSets();
		/// <summary>
		/// Sets up the descriptor pool for all resources.
		/// </summary>
		void setUpDescriptorPool();
		/// <summary>
		/// Tears-down the descriptor pool.
		/// </summary>
		void tearDownDescriptorPool();
		/// <summary>
		/// Prepares ray launcher layout, buffer and its memory.
		/// </summary>
		void setUpRayLauncher();
		/// <summary>
		/// Update ray launcher set. This isn't mutable, and should only be
		/// used once.
		/// </summary>
		void updateRayLauncherSet();
		/// <summary>
		/// Destroys the ray launcher layout, buffer and memory.
		/// </summary>
		void tearDownRayLauncher();
		/// <summary>
		/// Prepares rays and hits layout, buffers and memory.
		/// </summary>
		void setUpRaysAndHits();
		/// <summary>
		/// Update rays and hits set. This isn't mutable, and should only be
		/// used once.
		/// </summary>
		void updateRaysAndHitsSet();
		/// <summary>
		/// Destroys the rays and hits layout, buffers and memory.
		/// </summary>
		void tearDownRaysAndHits();
		/// <summary>
		/// Prepares pixels state layout, buffer and memory.
		/// </summary>
		void setUpPixelsState();
		/// <summary>
		/// Update pixels state set. This isn't mutable, and should only be
		/// used once.
		/// </summary>
		void updatePixelsStateSet();
		/// <summary>
		/// Destroys the pixels state layout, buffer and memory.
		/// </summary>
		void tearDownPixelsState();

		// ------------------------------------------------------------------ //
		// Pipelines.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Bulk sets-up all pipelines.
		/// </summary>
		void setUpPipelines(ThreadPool & thread_pool);
		/// <summary>
		/// Bulk tears-down all set-up pipelines.
		/// </summary>
		void tearDownPipelines();
		/// <summary>
		/// Prepares the ray generation layout, shader module and pipeline.
		/// </summary>
		void setUpGenPipeline();
		/// <summary>
		/// Destroys the ray generation pipeline, layout 
		/// </summary>
		void tearDownGenPipeline();
		/// <summary>
		/// Prepares the intersect layout, shader module and pipeline.
		/// </summary>
		void setUpIntersectPipeline();
		/// <summary>
		/// Destroys the intersect pipeline, layout 
		/// </summary>
		void tearDownIntersectPipeline();
		/// <summary>
		/// Prepares the colour layout, shader module and pipeline.
		/// </summary>
		void setUpColourPipeline();
		/// <summary>
		/// Destroys the colour pipeline, layout 
		/// </summary>
		void tearDownColourPipeline();
		/// <summary>
		/// Prepares the scatter layout, shader module and pipeline.
		/// </summary>
		void setUpScatterPipeline();
		/// <summary>
		/// Destroys the scatter pipeline, layout 
		/// </summary>
		void tearDownScatterPipeline();
		/// <summary>
		/// Prepares the post-processing layout, shader module and pipeline.
		/// </summary>
		void setUpPostProcessPipeline();
		/// <summary>
		/// Destroys the post-processing pipeline, layout 
		/// </summary>
		void tearDownPostProcessPipeline();
	};
}

#endif