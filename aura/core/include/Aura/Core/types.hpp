// ========================================================================== //
// File : types.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_TYPES
#define AURACORE_TYPES
// Internal includes.
// Standard includes.
#include <cstdint>
#include <type_traits>
// External includes.
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura
{
	/// <summary>
	/// Aura core namespace.
	/// </summary>
	namespace Core
	{
		/// <summary>
		/// Set if GPU should or not use doubles instead of floats.
		/// </summary>
		static constexpr bool gpu_double { false };
		/// <summary>
		/// GPU real type.
		/// </summary>
		using gpu_real = std::conditional<gpu_double, double, float>::type;
		/// <summary>
		/// GPU vec3 type.
		/// </summary>
		using gpu_vec3 = std::conditional<gpu_double, glm::dvec3, glm::vec3>::type;
		/// <summary>
		/// GPU vec4 type.
		/// </summary>
		using gpu_vec4 = std::conditional<gpu_double, glm::dvec4, glm::vec4>::type;
		/// <summary>
		/// GPU mat4 type.
		/// </summary>
		using gpu_mat4 = std::conditional<gpu_double, glm::dmat4, glm::mat4>::type;
	}
}

#endif
