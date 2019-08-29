// ========================================================================== //
// File : structures.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER_STRUCTURES
#define AURACORE_RENDER_STRUCTURES
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <cstdint>
// External includes.
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/// <summary>
/// Aura core environment namespace.
/// </summary>
namespace Aura::Core::Render
{
#pragma warning(disable : 4324)
	struct RayLauncher
	{
		alignas( sizeof(gpu_vec4) ) gpu_vec3 origin = gpu_vec3(0.0, 0.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 corner = gpu_vec3(0.0, 0.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 horizontal = gpu_vec3(1.0, 0.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 vertical = gpu_vec3(0.0, 1.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 u = gpu_vec3(1.0, 0.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 v = gpu_vec3(0.0, 1.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 w = gpu_vec3(0.0, 0.0, 1.0);
		gpu_real lens_radius = 1.0;
	};

	struct Ray
	{
		alignas( sizeof(gpu_vec4) ) gpu_vec3 origin = gpu_vec3(0.0, 0.0, 0.0);
		alignas( sizeof(gpu_vec4) ) gpu_vec3 direction = gpu_vec3(0.0, 0.0, 1.0);
		gpu_vec4 albedo = gpu_vec4(1.0, 1.0, 1.0, 1.0);
		std::uint32_t bounces = 0U;
		std::uint32_t width_index = 0U;
		std::uint32_t height_index = 0U;
	};

	struct RandomPush
	{
		gpu_real randoms[3] = { 0.0, 0.0, 0.0 };
	};
#pragma warning(default : 4324)
}

#endif
