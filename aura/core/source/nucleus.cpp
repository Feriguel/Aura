// ========================================================================== //
// File : nucleus.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/nucleus.hpp>
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
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
// External includes.

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the base core and starts rendering.
	/// </summary>
	Nucleus::Nucleus(
		std::string const & app_name, std::uint16_t const & app_major,
		std::uint16_t const & app_minor, std::uint16_t const & app_patch
	) :
		ThreadPool(std::thread::hardware_concurrency()),
		app_info({ app_name, app_major, app_minor, app_patch }),
		ui(*this), environment(*this), render(*this),
		frame_counter(0U), frame_limit(0U), stop(true)
	{
		loadDisplaySettings();
		ui.updateWindow();
		render.createSurface();
		render.checkPhysicalDevices();
		render.selectPhysicalDevice();
		render.createDevice();
		render.createFramework();
		renderStart();
	}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Nucleus::~Nucleus() noexcept
	{
		renderStop();
		render.waitIdle();
		{
			std::unique_lock<std::mutex> lock { stop_guard };
			stop_event.wait(lock, [&] { return !rendering; });
		}
		render.destroyFramework();
		render.destroyDevice();
		render.destroySurface();
	}
	// ------------------------------------------------------------------ //
	// Program control.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Program main loop, the maximum argument limit the total frames the
	/// render will process, 0 (default) is no limits (ignored in release).
	/// Can be used in a separate thread to permit mid run environment
	/// transformations.
	/// </summary>
	std::uint32_t Nucleus::run(std::uint32_t const max_frames)
	{
		if(stop) { renderStart(); }
		frameCounterReset(max_frames);
		while(!ui.shouldWindowClose())
		{
			ui.poolEvents();

			if(renderShouldStop()) { break; }
		}
		return frame_counter;
	}
	/// <summary>
	/// Frame render cycle. Queries for changes in environment, and updates
	/// the GPU is there are any changes. Then calls renderFrame to render
	/// and display a new frame.
	/// </summary>
	void Nucleus::renderCycle()
	{
		{
			std::unique_lock<std::mutex> lock { stop_guard };
			rendering = true;
		}
		while(true)
		{
			if(renderShouldStop()) { break; }
			bool result { render.renderFrame() };
			if(frame_limit == 0) { continue; }
			if(result) { frameCounterIncrement(); }
			if(frameCounterCheck()) { renderStop(); }
		}
		{
			std::unique_lock<std::mutex> lock { stop_guard };
			rendering = false;
			stop_event.notify_all();
		}
	}
	/// <summary>
	/// Resets the frame counter and updates the limit.
	/// </summary>
	void Nucleus::frameCounterReset(std::uint32_t const & max_frames) noexcept
	{
		std::unique_lock<std::mutex> lock { frame_guard };
		frame_limit = max_frames;
		frame_counter = 0;
	}
	/// <summary>
	/// Checks if the frame counter reached or surpassed the limit.
	/// </summary>
	bool Nucleus::frameCounterCheck() noexcept
	{
		std::unique_lock<std::mutex> lock { frame_guard };
		return frame_limit <= frame_counter;
	}
	/// <summary>
	/// Increments the frame counter.
	/// </summary>
	void Nucleus::frameCounterIncrement() noexcept
	{
		std::unique_lock<std::mutex> lock { frame_guard };
		++frame_counter;
	}
	/// <summary>
	/// Render thread start.
	/// </summary>
	void Nucleus::renderStart()
	{
		{
			std::unique_lock<std::mutex> lock { stop_guard };
			stop = false;
		}
		auto f_void { enqueue([&] { renderCycle(); }) };
	}
	/// <summary>
	/// Checks if render should stop.
	/// </summary>
	bool Nucleus::renderShouldStop() noexcept
	{
		std::unique_lock<std::mutex> lock { stop_guard };
		return stop;
	}
	/// <summary>
	/// Render thread stop.
	/// </summary>
	void Nucleus::renderStop() noexcept
	{
		std::unique_lock<std::mutex> lock { stop_guard };
		stop = true;
	}
	// ------------------------------------------------------------------ //
	// Nucleus update calls.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Updates the debug settings.
	/// </summary>
	void Nucleus::updateDebugSettings(DebugSettings const & new_settings)
	{
		debug_settings = new_settings;
	}
	/// <summary>
	/// Updates the display settings. 
	/// Changing device require a render re-build, changing window mode or
	/// it's dimensions requires an window reset and a render re-build.
	/// Both changes might take some time.
	/// </summary>
	void Nucleus::updateDisplaySettings(DisplaySettings const & new_settings)
	{
		bool window_reset { false };
		bool device_reset { false };
		{
			window_reset =
				new_settings.window_mode != display_settings.window_mode ||
				new_settings.width != display_settings.width ||
				new_settings.height != display_settings.height;
			device_reset =
				new_settings.device_name != display_settings.device_name;
		}
		if(!window_reset && !device_reset)
		{
			display_settings = new_settings;
			return;
		}
		renderStop();
		render.waitIdle();
		{
			std::unique_lock<std::mutex> lock { stop_guard };
			stop_event.wait(lock, [&] { return !rendering; });
		}
		render.destroyFramework();
		render.destroyDevice();
		if(window_reset)
		{
			render.destroySurface();
		}
		display_settings = new_settings;
		if(window_reset)
		{
			ui.updateWindow();
			render.createSurface();
			render.checkPhysicalDevices();
		}
		render.selectPhysicalDevice();
		render.createDevice();
		render.createFramework();
		renderStart();
	}
	/// <summary>
	/// Loads saved display settings.
	/// TODO Not Implemented Yet.
	/// </summary>
	void Nucleus::loadDisplaySettings()
	{}
}
