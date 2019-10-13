// ========================================================================== //
// File : main.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
#include <Aura/Core/nucleus.hpp>
// Standard includes.
#include <cstdint>
#include <chrono>
#include <mutex>
#include <future>
#include <thread>
#include <shared_mutex>
#include <string>
// External includes.
#pragma warning(disable : 26495 26812)
#include <gtest/gtest.h>
#pragma warning(default : 26495 26812)
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#pragma warning(disable : 26812)
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#pragma warning(default : 26812)

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
	TEST_F(CoreEnv, BuildSixtyFrameCornellBox)
	{
		std::uint32_t m_idx { 0U };
		std::uint32_t v_idx { 0U };
		std::uint32_t e_idx { 0U };

		// Floor
		{
			Core::Primitive primitive
			{
				Core::Primitive::Types::Cuboid, 0U, 0U, 0.0f,
				glm::uvec4(0U, 0U, 0U, 0U)
			};
			Core::Vertex v0 { glm::vec3(-2.1f, -2.1f, -2.1f) };
			Core::Vertex v1 { glm::vec3(2.1f, -2.0f, 2.1f) };
			Core::Material material
			{
				glm::vec4(0.9f, 0.9f, 0.9f, 2.0f),
				Core::Material::Types::Specular,
				1, 1.3f, 0.0f
			};
			ASSERT_TRUE(core->environment.newMaterial(material, m_idx));
			ASSERT_TRUE(core->environment.newEntity(m_idx, e_idx));
			ASSERT_TRUE(core->environment.newVertex(v0, v_idx));
			primitive.vertices.x = v_idx;
			ASSERT_TRUE(core->environment.newVertex(v1, v_idx));
			primitive.vertices.y = v_idx;
			ASSERT_TRUE(core->environment.entityAddPrimitive(e_idx, primitive));
		}
		// Roof
		{
			Core::Primitive primitive
			{
				Core::Primitive::Types::Cuboid, 0U, 0U, 0.0f,
				glm::uvec4(0U, 0U, 0U, 0U)
			};
			Core::Vertex v0 { glm::vec3(-2.1f, 2.0f, -2.1f) };
			Core::Vertex v1 { glm::vec3(2.1f, 2.1f, 2.1f) };
			Core::Material material
			{
				glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
				Core::Material::Types::Specular,
				0, 1.3f, 0.0f
			};
			ASSERT_TRUE(core->environment.newMaterial(material, m_idx));
			ASSERT_TRUE(core->environment.newEntity(m_idx, e_idx));
			ASSERT_TRUE(core->environment.newVertex(v0, v_idx));
			primitive.vertices.x = v_idx;
			ASSERT_TRUE(core->environment.newVertex(v1, v_idx));
			primitive.vertices.y = v_idx;
			ASSERT_TRUE(core->environment.entityAddPrimitive(e_idx, primitive));
		}
		// Cube
		{
			Core::Material material
			{
				glm::vec4(0.9f, 0.1f, 0.1f, 1.0f),
				Core::Material::Types::Diffuse,
				0, 1.3f, 0.5f
			};
			ASSERT_TRUE(core->environment.newMaterial(material, m_idx));
			ASSERT_TRUE(core->environment.newEntity(m_idx, e_idx));
			ASSERT_TRUE(core->environment.entityLoadModel(e_idx, "models/cube.obj"));
			core->environment.entityScale(e_idx, glm::vec3(0.75f, 0.75f, 0.75f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.5f, 0.0f, 0.5f));
			core->environment.entityRotate(e_idx, glm::vec3(0.5f, 0.5f, 5.0f));
		}
	}
	TEST_F(CoreEnv, PrimarySixtyFrameLoop)
	{
		{
			std::shared_lock<std::shared_mutex> scene_lock(core->environment.guard);
			{
				std::unique_lock<std::mutex> camera_lock(core->environment.scene->camera.guard);
				core->environment.scene->camera.data.look_from = glm::vec3(0.0f, 0.0f, 4.0f);
				core->environment.scene->camera.data.look_at = glm::vec3(0.0f, 0.0f, 0.0f);
				core->environment.scene->camera.data.v_up = glm::vec3(0.0f, 1.0f, 0.0f);
				core->environment.scene->camera.data.aperture = 0.001f;
				core->environment.scene->camera.data.focus = 1.0f;
				core->environment.scene->camera.updated = true;
			}
		}
		core->run(60U);
		ASSERT_TRUE(core->frame_counter >= 60U);
	}
	TEST_F(CoreEnv, SecondarySixtyFrameLoop)
	{
		core->run(60U);
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


