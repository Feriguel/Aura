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
// Standard includes.
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
	};
}

#endif
