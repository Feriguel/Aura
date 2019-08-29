// ========================================================================== //
// File : ui.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_UI
#define AURACORE_UI
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <string>
// External includes.
#include <GLFW/glfw3.h>

namespace Aura::Core
{
	// Engine core module.
	class Nucleus;
	// Handles GLFW window.
	class UIWindow;
	/// <summary>
	/// Holds the window and all user related inputs and outputs.
	/// </summary>
	class UI
	{
		// Holds the message to be thrown in case of a GLFW error.
		static inline std::string error_message {};
		// Nucleus handler.
		Nucleus & core_nucleus;
		// Window handler.
		UIWindow * window;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Initializes GLFW and its error callback and checks for vulkan support.
		/// </summary>
		explicit UI(Nucleus & nucleus);
		/// <summary>
		/// Terminates GLFW.
		/// </summary>
		~UI() noexcept;

		// ------------------------------------------------------------------ //
		// Error callback.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Definition of GLFW internal callback. Throws the error description,
		/// including the error code.
		/// </summary>
		static void errorCallback(int error, char const * description);

		// ------------------------------------------------------------------ //
		// Events.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Pool any pending events.
		/// </summary>
		void poolEvents() noexcept;

		// ------------------------------------------------------------------ //
		// Window.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Retrieves the created window, or null pointer in none was created.
		/// </summary>
		GLFWwindow * getWindow() noexcept;
		/// <summary>
		/// Destroys any existing window and creates a new window according
		/// given settings. Changes settings window dimensions if new window is
		/// in full-screen or windowed full-screen. Must be externally sync.
		/// </summary>
		void updateWindow();
		/// <summary>
		/// Checks if the window should close.
		/// </summary>
		bool shouldWindowClose();
	};
}

#endif
