// ========================================================================== //
// File : environment.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/render.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
// External includes.

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the environment.
	/// </summary>
	Environment::Environment(Nucleus & nucleus) :
		core_nucleus(nucleus), scene(nullptr)
	{
		scene = new Scene();
	}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Environment::~Environment() noexcept
	{
		delete scene;
	}

	// ------------------------------------------------------------------ //
	// Scene modification.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Replaces created scene with the new given scene. Only elements flagged
	/// with updated will be directly loaded.
	/// </summary>
	void Environment::replaceScene(Scene * const new_scene)
	{
		Scene * old_scene = scene;
		{
			std::unique_lock<std::shared_mutex> lock(guard);
			scene = new_scene;
		}
		delete old_scene;
	}
}
