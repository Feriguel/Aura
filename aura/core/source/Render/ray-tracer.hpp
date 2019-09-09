// ========================================================================== //
// File : ray_tracer.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RAY_TRACER
#define AURACORE_RAY_TRACER
// Internal includes.
#include <Aura/Core/types.hpp>
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
	/// Structure containing all the layout and set of a chain image.
	/// </summary>
	struct ChainResource
	{
		// Resource descriptor set layout.
		vk::DescriptorSetLayout set_layout {};
		// Resource descriptor set handles.
		vk::DescriptorSet set {};
	};
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
		static constexpr std::uint32_t const raygen_gsize[3] = { 8U, 8U, 1U };
		// Shader folder location
		static constexpr char const * shader_folder = "../aura/core/shaders/";
		// Scene access shared guard.
		std::shared_mutex & scene_guard;
		// Current scene
		Scene * const & scene;
		// Compute family index.
		std::uint32_t const compute_family;
		// Transfer family index.
		std::uint32_t const transfer_family;
		// Vulkan descriptor pool.
		vk::DescriptorPool pool;
		// Chain images resource.
		ChainResource chain_image;
		// Ray launcher resource.
		Resource ray_launcher;
		// Ray Generation pipeline.
		Pipeline gen;

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
			std::shared_mutex & scene_guard, Scene * const & scene);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~RayTracer() noexcept;

		// ------------------------------------------------------------------ //
		// Recordings.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Records a ray gen operation.
		/// </summary>
		void recordRayGen(std::uint32_t const & width, std::uint32_t const & height,
			RandomOffset & push, vk::CommandBuffer const & command) const;

		// ------------------------------------------------------------------ //
		// Resource updates.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Updates the ray launcher using the camera in the scene.
		/// </summary>
		void updateRayLauncher(std::uint32_t const & width, std::uint32_t const & height);

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
		/// Prepares chain image layout.
		/// </summary>
		void setUpChainImage();
		public:
		/// <summary>
		/// Updates chain image according to the frame index.
		/// </summary>
		void updateChainImageSet(std::uint32_t frame_index);
		private:
		/// <summary>
		/// Destroys chain image layout.
		/// </summary>
		void tearDownChainImage();
		/// <summary>
		/// Prepares ray launcher layout, buffer and its memory.
		/// </summary>
		void setUpRayLauncher();
		/// <summary>
		/// Update ray launcher set. This isn't mutable, and can only be
		/// used once.
		/// </summary>
		void updateRayLauncherSet();
		/// <summary>
		/// Destroys the ray launcher layout, buffer and its memory.
		/// </summary>
		void tearDownRayLauncher();

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
		/// Prepares the ray generation layout and shader module.
		/// </summary>
		void setUpGenPipeline();
		/// <summary>
		/// Destroys the ray generation pipeline, layout 
		/// </summary>
		void tearDownGenPipeline();
	};
}

#endif