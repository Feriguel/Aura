// ========================================================================== //
// File : ui.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/ui.hpp>
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
#include "UI/window.hpp"
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
		core_nucleus(nucleus)
	{
		// Set error callback.
		glfwSetErrorCallback(UI::errorCallback);
		// Initialises GLFW library.
		if(!glfwInit())
		{ throw std::exception(error_message.c_str()); }
		// Check Vulkan support.
		if(!glfwVulkanSupported())
		{ throw std::exception(error_message.c_str()); }
		// Initialises the window handler.
		window = new UIWindow();
	}
	/// <summary>
	/// Terminates GLFW.
	/// </summary>
	UI::~UI() noexcept
	{
		// Destroys the window handler.
		delete window;
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
	{ glfwPollEvents(); }

	// ------------------------------------------------------------------ //
	// Window.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Retrieves the created window, or null pointer in none was created.
	/// </summary>
	GLFWwindow * UI::getWindow() noexcept
	{ return window->getWindow(); }
	/// <summary>
	/// Destroys any existing window and creates a new window according
	/// given settings. Changes settings window dimensions if new window is
	/// in full-screen or windowed full-screen. Must be externally sync.
	/// </summary>
	void UI::updateWindow()
	{
		if(window->getWindow())
		{ window->destroyWindow(); }
		bool success = window->createWindow(
			core_nucleus.app_info.name, core_nucleus.display_settings);
		if(!success)
		{ throw std::exception(error_message.c_str()); }
	}
	/// <summary>
	/// Checks if the window should close.
	/// </summary>
	bool UI::shouldWindowClose()
	{ return glfwWindowShouldClose(window->getWindow()); }
}
