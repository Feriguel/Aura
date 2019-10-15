// ========================================================================== //
// File : ui.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_UI
#define AURACORE_UI
// Internal includes.
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <string>
// External includes.
#include <GLFW/glfw3.h>

namespace Aura::Core
{
	// Engine core module.
	class Nucleus;
	/// <summary>
	/// Holds the window and all user related inputs and outputs.
	/// </summary>
	class UI
	{
		// Holds the message to be thrown in case of a GLFW error.
		static inline std::string error_message {};
		// Nucleus handler.
		Nucleus & core_nucleus;
		public:
		// Window handler.
		GLFWwindow * window;

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
		/// Destroys any existing window and creates a new window according
		/// given settings. Changes settings window dimensions if new window is
		/// in full-screen or windowed full-screen. Must be externally sync.
		/// </summary>
		void updateWindow();
		/// <summary>
		/// Checks if the window should close.
		/// </summary>
		bool shouldWindowClose();
		/// <summary>
		/// Sets the window close flag.
		/// </summary>
		void setWindowCloseFlag(bool const & close);
		private:
		/// <summary>
		/// Creates a window according given settings. Changes settings window
		/// dimensions if in full-screen or windowed full-screen.
		/// Returns window creation success.
		/// </summary>
		bool createWindow(std::string const & window_name) noexcept;
		/// <summary>
		/// Destroys any created window and sets the pointer to null.
		/// </summary>
		void destroyWindow() noexcept;
		/// <summary>
		/// Creates a windowed window.
		/// </summary>
		bool createWindowedWindow(std::string const & window_name) noexcept;
		/// <summary>
		/// Creates a full-screen window, can be border-less or not.
		/// </summary>
		bool createFullScreenWindow(std::string const & window_name) noexcept;
	};
}

#endif
