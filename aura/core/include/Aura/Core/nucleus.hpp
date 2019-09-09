// ========================================================================== //
// File : nucleus.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_NUCLEUS
#define AURACORE_NUCLEUS
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Utilities/info.hpp>
#include <Aura/Core/Utilities/rng.hpp>
#include <Aura/Core/Utilities/thread_pool.hpp>
#include <Aura/Core/ui.hpp>
#include <Aura/Core/environment.hpp>
#include <Aura/Core/render.hpp>
// Standard includes.
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
// External includes.

namespace Aura::Core
{
	/// <summary>
	/// Engine core module, it covers the program cycle and sets up all
	/// other modules required for the application.
	/// </summary>
	class Nucleus : public ThreadPool, public RNGesus
	{
		// Current application info.
		Info const app_info;
		// Current debug settings and information.
		DebugSettings debug_settings;
		// Current display settings.
		DisplaySettings display_settings;
		// Holds the window and all user related inputs callbacks.
		UI ui;
		// Holds the current loaded scene.
		Environment environment;
		// Holds the program render and render cycle.
		Render render;
		// Render frame counter guard.
		std::mutex frame_guard;
		// Render frame counter.
		std::uint32_t frame_counter;
		// Render frame counter limit.
		std::uint32_t frame_limit;
		// Render stop guard.
		std::mutex stop_guard;
		// Rendering flag.
		std::condition_variable stop_event;
		// Render stop flag.
		bool stop;
		// Rendering cycle situation.
		bool rendering;

		// All main modules can access the all nucleus parts.
		friend class UI;
		friend class Environment;
		friend class Render;
		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the base core and starts rendering.
		/// </summary>
		explicit Nucleus(
			std::string const & app_name, std::uint16_t const & app_major,
			std::uint16_t const & app_minor, std::uint16_t const & app_patch);
		/// <summary>
		/// Stops rendering and tears-down the core.
		/// </summary>
		~Nucleus() noexcept;

		// ------------------------------------------------------------------ //
		// Program control.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Program main loop, the maximum argument limit the total frames the
		/// render will process, 0 (default) is no limits (ignored in release).
		/// Can be used in a separate thread to permit mid run environment
		/// transformations.
		/// Returns the frame counter value at stop condition.
		/// </summary>
		std::uint32_t run(std::uint32_t const max_frames = 0U);
		private:
		/// <summary>
		/// Frame render cycle. Queries for changes in environment, and updates
		/// the GPU is there are any changes. Then calls renderFrame to render
		/// and display a new frame.
		/// </summary>
		void renderCycle();
		/// <summary>
		/// Resets the frame counter and updates the limit.
		/// </summary>
		void frameCounterReset(std::uint32_t const & max_frames) noexcept;
		/// <summary>
		/// Checks if the frame counter reached or surpassed the limit.
		/// </summary>
		bool frameCounterCheck() noexcept;
		/// <summary>
		/// Increments the frame counter.
		/// </summary>
		void frameCounterIncrement() noexcept;
		/// <summary>
		/// Render thread start.
		/// </summary>
		void renderStart();
		/// <summary>
		/// Checks if render should stop.
		/// </summary>
		bool renderShouldStop() noexcept;
		/// <summary>
		/// Render thread stop.
		/// </summary>
		void renderStop() noexcept;

		// ------------------------------------------------------------------ //
		// Nucleus update calls.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Updates the debug settings.
		/// </summary>
		void updateDebugSettings(DebugSettings const & new_settings);
		/// <summary>
		/// Updates the display settings. 
		/// Changing device require a render re-build, changing window mode or
		/// it's dimensions requires an window reset and a render re-build.
		/// Both changes might take some time.
		/// </summary>
		void updateDisplaySettings(DisplaySettings const & new_settings);
		private:
		/// <summary>
		/// Loads saved display settings.
		/// </summary>
		void loadDisplaySettings();

		// ------------------------------------------------------------------ //
		// Obtain information.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Returns the current debug settings.
		/// </summary>
		DebugSettings const & getDebugSettings() const noexcept
		{ return debug_settings; }
		/// <summary>
		/// Returns the current display settings.
		/// </summary>
		DisplaySettings const & getDisplaySettings() const noexcept
		{ return display_settings; }
		/// <summary>
		/// Returns the application information.
		/// </summary>
		Info const & getAppInfo() const noexcept
		{ return app_info; }
		/// <summary>
		/// Returns the list of devices which support the application.
		/// </summary>
		std::vector<std::pair<vk::PhysicalDevice, bool>> const & getDevices() const noexcept
		{ return render.getPhysicalDevices(); }
	};
}

#endif
