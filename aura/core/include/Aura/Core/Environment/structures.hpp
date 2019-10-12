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
#include <vector>
// External includes.
#pragma warning(disable : 26812)
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
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
		/// Vertex information.
		/// </summary>
		struct Vertex
		{
			// Vertex position.
			alignas(sizeof(glm::vec4)) glm::vec3 position { 0.0f, 0.0f, 0.0f };
		};
		/// <summary>
		/// Contains the current transformation values.
		/// </summary>
		struct Transform
		{
			// Translation matrix.
			glm::mat4 translation { glm::identity<glm::mat4>() };
			// Scale matrix.
			glm::mat4 scaling { glm::identity<glm::mat4>() };
			// Full rotation matrix. All axis based rotations included.
			glm::mat4 rotation { glm::identity<glm::mat4>() };
		};
		/// <summary>
		/// Material description.
		/// </summary>
		struct Material
		{
			/// <summary>
			/// Enumeration of material types.
			/// </summary>
			enum struct Types : std::uint32_t
			{ Bounding = 0U, Test = 1U, Diffuse = 2U, Specular = 3U };
			// Material colour, alpha channel determines transparency.
			glm::vec4 albedo { 0.0f, 0.0f, 0.0f, 0.0f };
			// Type of material.
			Material::Types type { 0U };
			// Emission flag.
			std::uint32_t emissive { 0U };
			// Index of refraction relative to the air (1.0), used upon scattering.
			float refractive_index { 0.0f };
			// Ray scatter spread on specular surfaces.
			float fuzziness { 0.0f };
		};
		/// <summary>
		/// Definition of a hittable surface within the ray tracer.
		/// </summary>
		struct Primitive
		{
			/// <summary>
			/// Enumeration of primitive types.
			/// </summary>
			enum struct Types : std::uint32_t
			{ Empty = 0U, Sphere = 1U, Cuboid = 2U, Triangle = 3U };
			// Type of primitive.
			Primitive::Types type { Primitive::Types::Empty };
			// Index of primitive transformation.
			std::uint32_t transform_idx { 0U };
			// Index of primitive material.
			std::uint32_t material_idx { 0U };
			// Radius of sphere type.
			float radius { 0.0f };
			// Indices of composing vertices.
			glm::uvec4 vertices { 0U, 0U, 0U, 0U };
		};
		/// <summary>
		/// Aggregation of all elements that compromise an entity.
		/// </summary>
		struct Entity
		{
			// Entity transform.
			std::uint32_t transform_idx { 0U };
			// Entity Material.
			std::uint32_t material_idx { 0U };
			// Entity primitive list.
			std::vector<Primitive *> primitives {};
		};
		/// <summary>
		/// Physical representation of a point of view.
		/// </summary>
		struct Camera
		{
			// Camera origin.
			glm::vec3 look_from { 0.0f, 0.0f, 0.0f };
			// Camera look direction.
			glm::vec3 look_at { 0.0f, 0.0f, 1.0f };
			// Camera orientation.
			glm::vec3 v_up { 0.0f, 1.0f, 0.0f };
			// Current field of view [degree].
			float v_fov { 45.0f };
			// Aperture diameter.
			float aperture { 0.0f };
			// Focus distance.
			float focus { 1.0f };
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
			// List of scene vertices.
			UpdateGuard<std::vector<Vertex>> vertices {};
			// List of scene transforms.
			UpdateGuard<std::vector<Transform>> transforms {};
			// List of scene materials.
			UpdateGuard<std::vector<Material>> materials {};
			// List of scene primitives.
			UpdateGuard<std::vector<Primitive>> primitives {};
			// List of scene entities.
			UpdateGuard<std::vector<Entity>> entities {};
		};
	}
#	pragma warning(default : 4324)
}

#endif
