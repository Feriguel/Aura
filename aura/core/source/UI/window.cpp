// ========================================================================== //
// File : window.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include "UI/window.hpp"
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <string>
// External includes.
#include <GLFW/glfw3.h>

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Destroys any created window on tear-down.
	/// </summary>
	UIWindow::~UIWindow() noexcept
	{
		destroyWindow();
	}

	// ------------------------------------------------------------------ //
	// Window managing.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Creates a window according given settings. Changes settings window
	/// dimensions if in full-screen or windowed full-screen.
	/// Returns window creation success.
	/// </summary>
	bool UIWindow::createWindow(std::string const & window_name,
		DisplaySettings & settings) noexcept
	{
		switch(settings.window_mode)
		{
		case WindowModes::Windowed:
			return createWindowedWindow(window_name, settings);
		default:
			return createFullScreenWindow(window_name, settings);
		}
	}
	/// <summary>
	/// Destroys any created window and sets the pointer to null.
	/// </summary>
	void UIWindow::destroyWindow() noexcept
	{
		if(window) { glfwDestroyWindow(window); };
		window = nullptr;
	}

	// ------------------------------------------------------------------ //
	// Window creation methods.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Creates a windowed window.
	/// </summary>
	bool UIWindow::createWindowedWindow(std::string const & window_name,
		DisplaySettings & settings) noexcept
	{
		// Set window options.
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// Create window.
		window = glfwCreateWindow(
			// Image dimensions and name.
			settings.width, settings.height, window_name.c_str(),
			// Monitor and shared window.
			nullptr, nullptr
		);
		return window;
	}
	/// <summary>
	/// Creates a full-screen window, can be border-less or not.
	/// </summary>
	bool UIWindow::createFullScreenWindow(std::string const & window_name,
		DisplaySettings & settings) noexcept
	{
		// Monitor where the windowed full-screen will be set.
		GLFWmonitor * glfw_monitor { glfwGetPrimaryMonitor() };
		if(!glfw_monitor) { return false; }
		// If border-less.
		if(settings.window_mode == WindowModes::Borderless)
		{
			// Full-screen video mode.
			GLFWvidmode const * glfw_video_mode { glfwGetVideoMode(glfw_monitor) };
			if(!glfw_video_mode) { return false; }
			// Set window options.
			glfwWindowHint(GLFW_RED_BITS, glfw_video_mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, glfw_video_mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, glfw_video_mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, glfw_video_mode->refreshRate);
			// Update width and height.
			settings.width = glfw_video_mode->width;
			settings.height = glfw_video_mode->height;
		}
		// Set window options.
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// Create window.
		window = glfwCreateWindow(
			// Image dimensions and name.
			settings.width, settings.height, window_name.c_str(),
			// Monitor and shared window.
			glfw_monitor, nullptr
		);
		return window;
	}
}
