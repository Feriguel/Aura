// ========================================================================== //
// File : structures.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_ENV_STRUCTURES
#define AURACORE_ENV_STRUCTURES
// Internal includes.
// Standard includes.
#include <cstdint>
#include <mutex>
// External includes.
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

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
		/// <summary>
		/// Contains the current transformation values.
		/// </summary>
		struct Transform
		{
			// Translation matrix.
			glm::mat4 translation { 1.0f };
			// Scale matrix.
			glm::mat4 scaling { 1.0f };
			// Full rotation matrix. All axis based rotations included.
			glm::mat4 rotation { 1.0f };
		};
		/// <summary>
		/// Physical representation of a point of view.
		/// </summary>
		struct Camera
		{
			// Camera origin.
			glm::vec3 look_from { 0.0f, 0.0f, -2.0f };
			// Camera look direction.
			glm::vec3 look_at { 0.0f, 0.0f, 0.0f };
			// Camera orientation.
			glm::vec3 v_up { 0.0f, 1.0f, 0.0f };
			// Current field of view [degree].
			float v_fov { 45.0f };
			// Aperture diameter.
			float aperture { 1.0f };
			// Focus distance.
			float focus { 2.0f };
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
}

#endif
