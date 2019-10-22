// ========================================================================== //
// File : settings.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_SETTINGS
#define AURACORE_SETTINGS
// Internal includes.
// Standard includes.
#include <cstdint>
#include <string>
// External includes.

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura
{
	/// <summary>
	/// Aura core environment namespace.
	/// </summary>
	namespace Core
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
			// Maximum stored cameras.
			static constexpr std::size_t limit_cameras { 10U };
			// Maximum stored transforms and entities.
			static constexpr std::size_t limit_entities { 10U };
			// Maximum stored transforms.
			static constexpr std::size_t limit_primitives { 2000U };
			// Maximum stored materials.
			static constexpr std::size_t limit_materials { 10U };
			// Maximum stored vertices.
			static constexpr std::size_t limit_vertices { 4000U };
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
			// Outputs the time taken to present each frame, in milliseconds.
			static constexpr bool frame_time { true };
			// Outputs the time to a file.
			static constexpr bool time_to_file { true };
			// When on it splits all buffers memories to separate structures.
			static constexpr bool split_memory { true };
		};

		/// <summary>
		/// Contains the display settings, such as window type and size and some
		/// changeable render options. 
		/// </summary>
		struct DisplaySettings
		{
			// Type of created window.
			WindowModes window_mode { WindowModes::Fullscreen };
			// Image width in pixels.
			std::uint32_t width { 1920U };
			// Image height in pixels.
			std::uint32_t height { 1080U };
			// Selected device name.
			std::string device_name {};
			// Anti-aliasing additional samples, if not 0 randomizes each pixel
			// output directions on each sample and adds results. If 0 AA is off
			// gen rays direction is fixed.
			std::uint32_t anti_aliasing { 1U };
			// Maximum bounces allowed for each ray.
			std::uint32_t ray_depth { 12U };
			// Minimum ray lifetime.
			float t_min { 0.0000001f };
			// Maximum ray lifetime.
			float t_max { 2500.0f };
		};
	}
}

#endif
