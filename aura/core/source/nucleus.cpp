// ========================================================================== //
// File : nucleus.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/nucleus.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Utilities/info.hpp>
#include <Aura/Core/Utilities/rng.hpp>
#include <Aura/Core/Utilities/thread_pool.hpp>
#include <Aura/Core/ui.hpp>
#include <Aura/Core/environment.hpp>
#include <Aura/Core/render.hpp>
// Standard includes.
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
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
		frame_counter(0U), frame_limit(0U), rendering(false)
	{
		loadDisplaySettings();
		setUp(true, true);
	}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Nucleus::~Nucleus() noexcept
	{
		render.waitIdle();
		tearDown(true, true);
	}
	/// <summary>
	/// Restarts the renderer.
	/// </summary>
	void Nucleus::setUp(bool const window_reset, bool const device_reset)
	{
		if(window_reset)
		{
			ui.updateWindow();
			render.createSurface();
			render.checkPhysicalDevices();
		}
		if(device_reset || window_reset)
		{
			render.selectPhysicalDevice();
			render.createDevice();
			render.createFramework();
		}
		render.setUpDispatch();
	}
	/// <summary>
	/// Restarts the renderer.
	/// </summary>
	void Nucleus::tearDown(bool const window_reset, bool const device_reset)
	{
		render.tearDownDispatch();
		if(device_reset || window_reset)
		{
			render.destroyFramework();
			render.destroyDevice();
		}
		if(window_reset)
		{
			render.destroySurface();
		}
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
	void Nucleus::run(std::uint32_t const max_frames)
	{
		if(ui.shouldWindowClose())
		{
			render.waitIdle();
			tearDown(true, true);
			setUp(true, true);
		}
		std::future<void> render_task {};
		frameCounterReset(max_frames);
		while(!ui.shouldWindowClose())
		{
			ui.poolEvents();
			if(!isRendering())
			{
				{
					std::unique_lock<std::mutex> lock { rendering_guard };
					rendering = true;
				}
				if constexpr(debug_settings.frame_time)
				{ render_task = enqueue([&] { renderWithTime(); }); }
				else
				{ render_task = enqueue([&] { renderFrame(); }); }
			}
			if(frameCounterCheck()) { break; }
		}
		if(isRendering())
		{ render_task.wait(); }
	}
	/// <summary>
	/// Resets the frame counter and updates the limit.
	/// </summary>
	void Nucleus::frameCounterReset(std::uint32_t const & max_frames) noexcept
	{
		std::unique_lock<std::mutex> lock { frame_guard };
		frame_limit = max_frames;
		frame_counter = 0U;
	}
	/// <summary>
	/// Checks if the frame counter reached or surpassed the limit.
	/// </summary>
	bool Nucleus::frameCounterCheck() noexcept
	{
		if(frame_limit != 0U)
		{
			std::unique_lock<std::mutex> lock { frame_guard };
			return frame_limit <= frame_counter;
		}
		return false;
	}
	/// <summary>
	/// Increments the frame counter.
	/// </summary>
	void Nucleus::frameCounterIncrement() noexcept
	{
		if(frame_limit != 0U)
		{
			std::unique_lock<std::mutex> lock { frame_guard };
			++frame_counter;
		}
	}
	/// <summary>
	/// Checks rendering flag.
	/// </summary>
	bool Nucleus::isRendering()
	{
		std::unique_lock<std::mutex> lock { rendering_guard };
		return rendering;
	}
	/// <summary>
	/// Schedule a rendering job and set rendering flag to true.
	/// </summary>
	void Nucleus::renderFrame()
	{
		if(!render.dispatchFrame())
		{
			std::unique_lock<std::mutex> lock { rendering_guard };
			rendering = false;
		}
		if(!render.waitForMainFence(std::numeric_limits<std::uint64_t>::max()))
		{
			std::unique_lock<std::mutex> render_lock { rendering_guard };
			rendering = false;
			std::unique_lock<std::mutex> frame_lock { frame_guard };
			frame_counter = frame_limit;
			throw std::exception("Render timeout.");
		}
		else
		{
			std::unique_lock<std::mutex> lock { rendering_guard };
			rendering = false;
		}
		frameCounterIncrement();
	}
	/// <summary>
	/// Schedule a rendering job, wait for it to stop and outputs time.
	/// </summary>
	void Nucleus::renderWithTime()
	{
		std::chrono::time_point<std::chrono::system_clock> start, end;
		start = std::chrono::system_clock::now();
		renderFrame();
		end = std::chrono::system_clock::now();
		std::cout << std::chrono::duration<double, std::milli>(end - start).count() << std::endl;
	}

	// ------------------------------------------------------------------ //
	// Nucleus update calls.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Updates the debug settings.
	/// </summary>
	void Nucleus::updateDebugSettings(DebugSettings new_settings)
	{
		debug_settings = std::move(new_settings);
	}
	/// <summary>
	/// Updates the display settings. 
	/// Changing device require a render re-build, changing window mode or
	/// it's dimensions requires an window reset and a render re-build.
	/// Both changes might take some time.
	/// </summary>
	void Nucleus::updateDisplaySettings(DisplaySettings new_settings)
	{
		bool window_reset, device_reset, sync_reset { false };
		{
			window_reset =
				new_settings.window_mode != display_settings.window_mode ||
				new_settings.width != display_settings.width ||
				new_settings.height != display_settings.height;
			device_reset =
				new_settings.device_name != display_settings.device_name;
			sync_reset =
				new_settings.anti_aliasing != display_settings.anti_aliasing ||
				new_settings.ray_depth != display_settings.ray_depth;
		}
		if(!window_reset && !device_reset && !sync_reset)
		{
			display_settings = std::move(new_settings);
			return;
		}
		render.waitIdle();
		tearDown(window_reset, device_reset);
		display_settings = std::move(new_settings);
		setUp(window_reset, device_reset);
	}
	/// <summary>
	/// Loads saved display settings.
	/// TODO Not Implemented Yet.
	/// </summary>
	void Nucleus::loadDisplaySettings()
	{}
}
