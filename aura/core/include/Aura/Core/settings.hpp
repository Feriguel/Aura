// ========================================================================== //
// File : settings.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_SETTINGS
#define AURACORE_SETTINGS
// Internal includes.
#include <Aura/Core/types.hpp>
// Standard includes.
#include <cstdint>
#include <string>
// External includes.

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Helper structures.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Program window mode types.
	/// </summary>
	enum struct WindowModes
	{
		Windowed,
		Borderless,
		Fullscreen
	};

	// ------------------------------------------------------------------ //
	// Pre-processor definitions and storage limits.
	// ------------------------------------------------------------------ //
#	if defined (NDEBUG)
	// Debug mode flag.
	static constexpr bool debug_mode { false };
#	else
	// Debug mode flag.
	static constexpr bool debug_mode { true };
#	endif
	/// <summary>
	/// Environment structure limits.
	/// </summary>
	struct EnvLimits
	{
		// Maximum stored transforms.
		static constexpr std::size_t limit_transforms { 5U };
		// Maximum stored cameras.
		static constexpr std::size_t limit_cameras { 5U };
	};

	// ------------------------------------------------------------------ //
	// Settings.
	// ------------------------------------------------------------------ //
	struct DebugSettings
	{
		// Initializes the vulkan instance with API dump, for easier debugging
		// Must be set before compilation. Disabled if not in debug mode.
		// This is extremely slow.
		static constexpr bool api_dump { false };
	};

	/// <summary>
	/// Contains the display settings, such as window type and size and some
	/// changeable render options. 
	/// </summary>
	struct DisplaySettings
	{
		// Type of created window.
		WindowModes window_mode { WindowModes::Windowed };
		// Image width in pixels.
		std::uint32_t width { 1280U };
		// Image height in pixels.
		std::uint32_t height { 720U };
		// Selected device name.
		std::string device_name {};
		// Anti-aliasing additional samples, if not 0 randomizes each pixel
		// output directions on each sample and adds results. If 0 AA is off
		// gen rays direction is fixed.
		std::uint32_t anti_aliasing { 0U };
	};
}

#endif
