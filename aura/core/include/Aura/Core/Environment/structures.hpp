// ========================================================================== //
// File : structures.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_ENV_STRUCTURES
#define AURACORE_ENV_STRUCTURES
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <cstdint>
// External includes.
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

/// <summary>
/// Aura core environment namespace.
/// </summary>
namespace Aura::Core::Environment
{
#pragma warning(disable : 4324)
	struct Transform
	{
		gpu_mat4 translation = gpu_mat4(1.0);
		gpu_mat4 scaling = gpu_mat4(1.0);
		gpu_mat4 rotation = gpu_mat4(1.0);
	};

	struct Camera
	{
		gpu_vec3 look_from = gpu_vec3(0.0, 2.0, -2.0);
		gpu_vec3 look_at = gpu_vec3(0.0, 0.0, 0.0);
		gpu_vec3 v_up = gpu_vec3(0.0, 1.0, 0.0);
		gpu_real v_fov = 45.0;
		gpu_real aperture = 1.0;
		gpu_real focus = 2.0;
	};
#pragma warning(default : 4324)
}

#endif
