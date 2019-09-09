// ========================================================================== //
// File : core.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_ENV
#define AURACORE_ENV
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
#include <shared_mutex>
// External includes.

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
		/// Replaces created scene with the new given scene.
		/// </summary>
		void replaceScene(Scene * const new_scene);
	};
}

#endif
