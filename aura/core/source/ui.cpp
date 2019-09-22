// ========================================================================== //
// File : ui.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/ui.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
// Standard includes.
#include <exception>
#include <string>
#include <sstream>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Initialises GLFW and its error callback checks for vulkan support.
	/// </summary>
	UI::UI(Nucleus & nucleus) :
		core_nucleus(nucleus), window(nullptr)
	{
		// Set error callback.
		glfwSetErrorCallback(UI::errorCallback);
		// Initialises GLFW library.
		if(!glfwInit())
		{ throw std::exception(error_message.c_str()); }
		// Check Vulkan support.
		if(!glfwVulkanSupported())
		{ throw std::exception(error_message.c_str()); }
	}
	/// <summary>
	/// Terminates GLFW.
	/// </summary>
	UI::~UI() noexcept
	{
		// Destroys the window handler.
		destroyWindow();
		// Terminates GLFW.
		glfwTerminate();
	}

	// ------------------------------------------------------------------ //
	// Error callback.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Definition of GLFW internal callback.
	/// Displays description, including the error code.
	/// </summary>
	void UI::errorCallback(int error, char const * description)
	{
		std::stringstream ss { "GLFW Error [" };
		ss << std::hex << error << "]: " << description;
		error_message = ss.str();
	}

	// ------------------------------------------------------------------ //
	// Events.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Pool any pending events.
	/// </summary>
	void UI::poolEvents() noexcept
	{
		glfwPollEvents();
	}

	// ------------------------------------------------------------------ //
	// Window.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Destroys any existing window and creates a new window according
	/// given settings. Changes settings window dimensions if new window is
	/// in full-screen or windowed full-screen. Must be externally sync.
	/// </summary>
	void UI::updateWindow()
	{
		if(window)
		{ destroyWindow(); }
		bool success = createWindow(core_nucleus.app_info.name);
		if(!success)
		{ throw std::exception(error_message.c_str()); }
	}
	/// <summary>
	/// Checks if the window should close.
	/// </summary>
	bool UI::shouldWindowClose()
	{
		return glfwWindowShouldClose(window);
	}
	/// <summary>
	/// Creates a window according given settings. Changes settings window
	/// dimensions if in full-screen or windowed full-screen.
	/// Returns window creation success.
	/// </summary>
	bool UI::createWindow(std::string const & window_name) noexcept
	{
		switch(core_nucleus.display_settings.window_mode)
		{
		case WindowModes::Windowed:
			return createWindowedWindow(window_name);
		default:
			return createFullScreenWindow(window_name);
		}
	}
	/// <summary>
	/// Destroys any created window and sets the pointer to null.
	/// </summary>
	void UI::destroyWindow() noexcept
	{
		if(window) { glfwDestroyWindow(window); };
		window = nullptr;
	}
	/// <summary>
	/// Creates a windowed window.
	/// </summary>
	bool UI::createWindowedWindow(std::string const & window_name) noexcept
	{
		// Get settings.
		DisplaySettings & settings { core_nucleus.display_settings };
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
	bool UI::createFullScreenWindow(std::string const & window_name) noexcept
	{
		// Get settings.
		DisplaySettings & settings { core_nucleus.display_settings };
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
