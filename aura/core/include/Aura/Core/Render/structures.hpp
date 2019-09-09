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
namespace Aura::Core
{
#pragma warning(disable : 4324)
	/// <summary>
	/// Representation of camera used within the shader to determine the rays
	/// origin and direction.
	/// </summary>
	struct RayLauncher
	{
		// Camera origin.
		alignas(sizeof(gpu_vec4)) gpu_vec3 origin { 0.0, 0.0, 0.0 };
		// Image top left corner.
		alignas(sizeof(gpu_vec4)) gpu_vec3 corner { 0.0, 0.0, 0.0 };
		// Image horizontal extension.
		alignas(sizeof(gpu_vec4)) gpu_vec3 horizontal { 1.0, 0.0, 0.0 };
		// Image vertical extension.
		alignas(sizeof(gpu_vec4)) gpu_vec3 vertical { 0.0, 1.0, 0.0 };
		// Pixel horizontal unit vector.
		alignas(sizeof(gpu_vec4)) gpu_vec3 u { 1.0, 0.0, 0.0 };
		// Pixel vertical unit vector.
		alignas(sizeof(gpu_vec4)) gpu_vec3 v { 0.0, 1.0, 0.0 };
		// Pixel depth unit vector.
		alignas(sizeof(gpu_vec4)) gpu_vec3 w { 0.0, 0.0, 1.0 };
		// Camera lens radius used to perform a scope effect.
		gpu_real lens_radius { 1.0 };
	};
#pragma warning(default : 4324)
	/// <summary>
	/// Structure which contains the random values supplied by push constants to
	/// the shader.
	/// </summary>
	struct RandomOffset
	{
		// Random values.
		gpu_real randoms[2] { 0.0, 0.0 };
	};
#pragma warning(disable : 4324)
	/// <summary>
	/// Ray full description.
	/// </summary>
	struct Ray
	{
		// Ray origin.
		alignas(sizeof(gpu_vec4)) gpu_vec3 origin { 0.0, 0.0, 0.0 };
		// Ray direction.
		alignas(sizeof(gpu_vec4)) gpu_vec3 direction { 0.0, 0.0, 1.0 };
		// Ray colour strength.
		gpu_vec4 albedo { 1.0, 1.0, 1.0, 1.0 };
		// Bounces performed by the ray.
		std::uint32_t bounces { 0U };
		// Image width corresponding to this ray.
		std::uint32_t width_index { 0U };
		// Image height corresponding to this ray.
		std::uint32_t height_index { 0U };
	};
#pragma warning(default : 4324)
}

#endif
