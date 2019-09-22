// ========================================================================== //
// File : main.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
// Standard includes.
#include <cstdint>
#include <thread>
#include <string>
// External includes.
#pragma warning(disable : 26495)
#include <gtest/gtest.h>
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

/// <summary>
/// Aura test namespace.
/// </summary>
namespace Aura::Test
{
	/// <summary>
	/// Core Environment test fixture.
	/// </summary>
	class CoreEnv : public ::testing::Test
	{
		protected:
		// Core nucleus instance.
		static inline Core::Nucleus * core = nullptr;
		// Per-test-suite set-up.
		// Called before the first test in this test suite.
		static void SetUpTestSuite()
		{
			std::string const app_name { "Test [π]" };
			std::uint16_t constexpr app_major { 1U };
			std::uint16_t constexpr app_minor { 0U };
			std::uint16_t constexpr app_patch { 0U };
			core = new Core::Nucleus(app_name, app_major, app_minor, app_patch);
		}
		// Per-test-suite tear-down.
		// Called after the last test in this test suite.
		static void TearDownTestSuite()
		{
			delete core;
		}
		// Per-test set-up logic.
		void SetUp() override
		{}
		// Per-test tear-down logic.
		void TearDown() override
		{}
	};

	// ------------------------------------------------------------------ //
	// Base tests.
	// ------------------------------------------------------------------ //
	TEST_F(CoreEnv, UpdateWindowSettings)
	{

		Core::DisplaySettings new_settings { core->getDisplaySettings() };
		new_settings.window_mode = Core::WindowModes::Windowed;
		new_settings.width = 1280;
		new_settings.height = 720;
		ASSERT_NO_THROW(core->updateDisplaySettings(new_settings));
	}
	TEST_F(CoreEnv, SixtyFrameLoop)
	{
		core->run(60U);
		ASSERT_TRUE(core->frame_counter >= 60U);
	}
	TEST_F(CoreEnv, InfLoop)
	{
		core->run();
	}
}

/// <summary>
/// Launches test application.
/// </summary>
int main(int argc, char * argv[])
{
	// Start google test.
	::testing::InitGoogleTest(&argc, argv);
	// Run all the tests.
	return RUN_ALL_TESTS();
}


