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
#include <mutex>
// External includes.
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

/// <summary>
/// Aura core environment namespace.
/// </summary>
namespace Aura::Core
{
	/// <summary>
	/// Contains the current transformation values.
	/// </summary>
	struct Transform
	{
		// Translation matrix.
		gpu_mat4 translation { 1.0 };
		// Scale matrix.
		gpu_mat4 scaling { 1.0 };
		// Full rotation matrix. All axis based rotations included.
		gpu_mat4 rotation { 1.0 };
	};
	/// <summary>
	/// Physical representation of a point of view.
	/// </summary>
	struct Camera
	{
		// Camera origin.
		gpu_vec3 look_from { 0.0, 0.0, -2.0 };
		// Camera look direction.
		gpu_vec3 look_at { 0.0, 0.0, 0.0 };
		// Camera orientation.
		gpu_vec3 v_up { 0.0, 1.0, 0.0 };
		// Current field of view [degree].
		gpu_real v_fov { 45.0 };
		// Aperture diameter.
		gpu_real aperture { 1.0 };
		// Focus distance.
		gpu_real focus { 2.0 };
		// Camera internal transform.
		Transform transform {};
	};
	/// <summary>
	/// Data update control and lock structure. Used to prevent data race and
	/// limit data transfers to GPU to only when updates exist.
	/// </summary>
	template <class DataType>
	struct UpdateGuard
	{
		// Access control.
		std::mutex guard {};
		// Update flag.
		bool updated { true };
		// Stored data.
		DataType data {};
	};
	/// <summary>
	/// Current scene representation. Contains all information regarding the 
	/// current stored scene. Every data storage has its own update guard.
	/// </summary>
	struct Scene
	{
		// Scene camera.
		UpdateGuard<Camera> camera {};
	};
}

#endif
