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
#include <array>
// External includes.
#pragma warning(disable : 26812)
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#pragma warning(default : 26812)

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
		/// Structure used in the uniform buffer to supply fixed settings into the
		/// shader trough all stages.
		/// </summary>
		struct RenderSettings
		{
			// Minimum accepted ray travel distance.
			float t_min = { 0U };
			// Maximum accepted ray travel distance.
			float t_max = { 0U };
			// Number bounces allowed per ray.
			std::uint32_t n_bounces = { 0U };
			// Number of samples per frame.
			std::uint32_t n_samples = { 0U };
			// Number of primitives in scene.
			std::uint32_t n_primitives = { 0U };
			// Image width.
			std::uint32_t width = { 0U };
			// Image height.
			std::uint32_t height = { 0U };
		};
		/// <summary>
		/// Structure which contains random values supplied by push constants to
		/// the shader.
		/// </summary>
		struct RandomSeed
		{
			glm::vec2 seed {};
		};
		/// <summary>
		/// Structure which contains random values supplied by push constants to
		/// the shader.
		/// </summary>
		struct RandomPointInCircleAndSeed
		{
			glm::vec3 point {};
			float seed {};
		};
		/// <summary>
		/// Representation of camera used within the shader to determine the rays
		/// origin and direction.
		/// </summary>
		struct RayLauncher
		{
			// Camera origin.
			glm::vec3 origin { 0.0f, 0.0f, 0.0f };
			// Camera lens radius used to perform a scope effect.
			float lens_radius { 1.0f };
			// Image top left corner.
			alignas(sizeof(glm::vec4)) glm::vec3 corner { 0.0f, 0.0f, 0.0f };
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
			// Ray origin.
			alignas(sizeof(glm::vec4)) glm::vec3 origin { 0.0f, 0.0f, 0.0f };
			// Ray direction.
			alignas(sizeof(glm::vec4)) glm::vec3 direction { 0.0f, 0.0f, 1.0f };
			// Ray colour strength.
			glm::vec3 albedo { 1.0f, 1.0f, 1.0f };
			// If ray has not missed.
			std::uint32_t missed { 0U };
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
			// Hit material index.
			std::uint32_t m_idx { 0U };
			// Inner hit flag.
			std::uint32_t inside { 0U };
		};
		/// <summary>
		/// Pixel state.
		/// </summary>
		struct Pixel
		{
			// Sum of all obtained colours.
			glm::vec4 colour { 0.0f, 0.0f, 0.0f, 0.0f };
		};
#	pragma warning(default : 4324)
	}
}

#endif
