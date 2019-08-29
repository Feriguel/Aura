// ========================================================================== //
// File : render.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include "ray-tracer.hpp"
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <cstdint>
#include <vector>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the ray tracer structure.
	/// </summary>
	RayTracer::RayTracer(
		vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
		vk::PhysicalDevice const & physical_device, vk::Device const & device,
		vk::SurfaceKHR const & surface, vk::Extent2D const & chain_base_extent,
		std::vector<std::uint32_t> const & chain_accessing_families) :
		VulkanCompute(dispatch, instance, physical_device, device),
		VulkanSwapchain(dispatch, instance, physical_device, device,
			surface, chain_base_extent, chain_accessing_families)
	{}
	/// <summary>
	/// Tears-down the ray tracer.
	/// </summary>
	RayTracer::~RayTracer() noexcept
	{}
}
