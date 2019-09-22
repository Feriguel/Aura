// ========================================================================== //
// File : structures.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER_STRUCTURES
#define AURACORE_RENDER_STRUCTURES
// Internal includes.
// Standard includes.
#include <cstdint>
// External includes.
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura
{
	/// <summary>
	/// Aura core environment namespace.
	/// </summary>
	namespace Core
	{
#	pragma warning(disable : 4324)
		/// <summary>
		/// Representation of camera used within the shader to determine the rays
		/// origin and direction.
		/// </summary>
		struct RayLauncher
		{
			// Camera origin.
			glm::vec3 origin { 0.0f, 0.0f, 0.0f };
			// Maximum bounces allowed per ray.
			std::uint32_t max_bounces = { 0U };
			// Image top left corner.
			glm::vec3 corner { 0.0f, 0.0f, 0.0f };
			// Camera lens radius used to perform a scope effect.
			float lens_radius { 1.0f };
			// Image horizontal extension.
			alignas(sizeof(glm::vec4)) glm::vec3 horizontal { 1.0f, 0.0f, 0.0f };
			// Image vertical extension.
			alignas(sizeof(glm::vec4)) glm::vec3 vertical { 0.0f, 1.0f, 0.0f };
			// Pixel horizontal unit vector.
			alignas(sizeof(glm::vec4)) glm::vec3 u { 1.0f, 0.0f, 0.0f };
			// Pixel vertical unit vector.
			alignas(sizeof(glm::vec4)) glm::vec3 v { 0.0f, 1.0f, 0.0f };
			// Pixel depth unit vector.
			alignas(sizeof(glm::vec4)) glm::vec3 w { 0.0f, 0.0f, 1.0f };
		};
		/// <summary>
		/// Ray full description.
		/// </summary>
		struct Ray
		{
			// Ray colour strength.
			glm::vec4 albedo { 1.0f, 1.0f, 1.0f, 1.0f };
			// Ray origin.
			glm::vec3 origin { 0.0f, 0.0f, 0.0f };
			// Bounces performed by the ray.
			std::uint32_t bounces { 0U };
			// Ray direction.
			alignas(sizeof(glm::vec4)) glm::vec3 direction { 0.0f, 0.0f, 1.0f };
		};
		/// <summary>
		/// Ray hit description.
		/// </summary>
		struct Hit
		{
			// Intersection point.
			glm::vec3 point { 0.0f, 0.0f, 0.0f };
			// Intersection time.
			float time { 1.0f };
			// Ray direction.
			glm::vec3 normal { 0.0f, 0.0f, 0.0f };
			// Bounces performed by the ray.
			std::uint32_t bounces { 0U };
		};
		/// <summary>
		/// Pixel state.
		/// </summary>
		struct Pixel
		{
			// Sum of all obtained colours.
			glm::vec4 colour { 0.0f, 0.0f, 0.0f, 0.0f };
			// Total bounces in this pixel.
			std::uint32_t bounces { 0U };
		};
#	pragma warning(default : 4324)
		/// <summary>
		/// Structure which contains the random values supplied by push constants to
		/// the shader.
		/// </summary>
		struct RandomOffset
		{
			// Horizontal offset.
			float s { 0.0f };
			// Vertical offset.
			float t { 0.0f };
		};
	}
}

#endif
