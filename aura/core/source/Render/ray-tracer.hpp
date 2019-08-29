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
#include "framework.hpp"
#include "compute.hpp"
#include "swapchain.hpp"
// Standard includes.
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
	/// <summary>
	/// 
	/// </summary>
	class RayTracer : public VulkanCompute, public VulkanSwapchain
	{
		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the base core and starts rendering.
		/// </summary>
		explicit RayTracer(
			vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
			vk::PhysicalDevice const & physical_device, vk::Device const & device,
			vk::SurfaceKHR const & surface, vk::Extent2D const & chain_base_extent,
			std::vector<std::uint32_t> const & chain_accessing_families);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~RayTracer() noexcept;
	};
}

#endif