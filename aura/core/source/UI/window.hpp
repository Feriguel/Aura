// ========================================================================== //
// File : window.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_UI_WINDOW
#define AURACORE_UI_WINDOW
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <cstdint>
#include <string>
// External includes.
#include <GLFW/glfw3.h>

namespace Aura::Core
{
	/// <summary>
	/// Handles GLFW window. Destroys any created window on tear-down.
	/// </summary>
	class UIWindow
	{
		// Window handler.
		GLFWwindow * window;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Destroys any created window on tear-down.
		/// </summary>
		~UIWindow() noexcept;

		// ------------------------------------------------------------------ //
		// Window managing.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Creates a window according given settings. Changes settings window
		/// dimensions if in full-screen or windowed full-screen.
		/// Returns window creation success.
		/// </summary>
		bool createWindow(std::string const & window_name,
			DisplaySettings & settings) noexcept;
		/// <summary>
		/// Destroys any created window and sets the pointer to null.
		/// </summary>
		void destroyWindow() noexcept;
		/// <summary>
		/// Retrieves the created window, or null pointer in none was created.
		/// </summary>
		constexpr GLFWwindow * getWindow() noexcept
		{ return window; }

		// ------------------------------------------------------------------ //
		// Window creation methods.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Creates a windowed window.
		/// </summary>
		bool createWindowedWindow(std::string const & window_name,
			DisplaySettings & settings) noexcept;
		/// <summary>
		/// Creates a full-screen window, can be border-less or not.
		/// </summary>
		bool createFullScreenWindow(std::string const & window_name,
			DisplaySettings & settings) noexcept;
	};
}

#endif
