// ========================================================================== //
// File : environment.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/render.hpp>
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
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
		core_nucleus(nucleus)
	{}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Environment::~Environment() noexcept
	{}
}
