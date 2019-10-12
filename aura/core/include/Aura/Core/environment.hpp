// ========================================================================== //
// File : core.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_ENV
#define AURACORE_ENV
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
#include <shared_mutex>
#include <string>
// External includes.
#pragma warning(disable : 26812)
#include <glm/vec3.hpp>
#pragma warning(default : 26812)

/// <summary>
/// Aura core namespace.
/// </summary>
namespace Aura::Core
{
	// Engine core module.
	class Nucleus;
	/// <summary>
	/// Contains a loaded scene and all modification calls.
	/// The loaded scene can be destroyed and recreated or exchanged at any time.
	/// All scene handlers creation and destruction are externally managed.
	/// </summary>
	class Environment
	{
		// Nucleus handler.
		Nucleus & core_nucleus;
		public:
		// Scene guard.
		std::shared_mutex guard;
		// Current loaded scene.
		Scene * scene;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the environment.
		/// </summary>
		explicit Environment(Nucleus & nucleus);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~Environment() noexcept;

		// ------------------------------------------------------------------ //
		// Scene modification.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Replaces created scene with the new given scene. Takes ownership of
		/// the new scene.
		/// </summary>
		void replaceScene(Scene * new_scene);
		private:
		/// <summary>
		/// Adds the given vertex to the scene. The vertex is moved to the
		/// returned address.
		/// </summary>
		Vertex * addVertex(Vertex & vertex, std::uint32_t & idx);
		/// <summary>
		/// Adds the given transform to the scene. The transform is moved to the
		/// returned address.
		/// </summary>
		Transform * addTransform(Transform & transform, std::uint32_t & idx);
		/// <summary>
		/// Adds the given primitive to the scene. The primitive is moved to the
		/// returned address.
		/// </summary>
		Primitive * addPrimitive(Primitive & primitive, std::uint32_t & idx);
		/// <summary>
		/// Adds the given material to the scene. The material is moved to the
		/// returned address.
		/// </summary>
		Material * addMaterial(Material & material, std::uint32_t & idx);
		/// <summary>
		/// Adds the given entity to the scene. The entity is moved to the
		/// returned address.
		/// </summary>
		Entity * addEntity(Entity & entity, std::uint32_t & idx);
		public:
		/// <summary>
		/// Creates a vertex and updates given index.
		/// Returns whenever a new vertex was created.
		/// </summary>
		bool newVertex(Vertex & vertex, std::uint32_t & idx);
		/// <summary>
		/// Creates a material and updates given index.
		/// Returns whenever a new material was created.
		/// </summary>
		bool newMaterial(Material & material, std::uint32_t & idx);
		/// <summary>
		/// Creates an empty entity and updates given index. Sets the entity
		/// material as the given material.
		/// Returns whenever a new entity was created.
		/// </summary>
		bool newEntity(std::uint32_t const material_idx, std::uint32_t & idx);
		/// <summary>
		/// Adds a the given primitive to the scene. The primitive's material
		/// and transform indices are ignored and it is moved into the scene. 
		/// </summary>
		bool entityAddPrimitive(std::uint32_t const entity_idx, Primitive & primitive);
		/// <summary>
		/// Sets material as entity's material.
		/// </summary>
		void entityMaterial(std::uint32_t const entity_idx, std::uint32_t const material_idx);
		/// <summary>
		/// Sets the entity translation matrix. Each vector component is a
		/// translation in respect to each axis.
		/// </summary>
		void entityTranslate(std::uint32_t const entity_idx, glm::vec3 const translate);
		/// <summary>
		/// Sets the entity scaling matrix. Each vector component is a scale
		/// in respect to each axis.
		/// </summary>
		void entityScale(std::uint32_t const entity_idx, glm::vec3 const scale);
		/// <summary>
		/// Sets the entity rotation matrix. Each vector component is a rotation
		/// in respect to each axis.
		/// </summary>
		void entityRotate(std::uint32_t const entity_idx, glm::vec3 const rotate);
		/// <summary>
		/// Loads the model at path into the given entity.
		/// </summary>
		bool entityLoadModel(std::uint32_t const entity_idx, std::string const path);
	};
}

#endif
