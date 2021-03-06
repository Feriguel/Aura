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

		/// <summary>
		/// Auxiliary cuboid insert.
		/// </summary>
		bool addCuboid(Core::Vertex v0, Core::Vertex v1, Core::Material material, std::uint32_t & e_idx)
		{
			std::uint32_t tmp_idx { 0U };
			Core::Primitive primitive
			{
				Core::Primitive::Types::Cuboid, 0U, 0U, 0.0f,
				glm::uvec4(0U, 0U, 0U, 0U)
			};
			if(!core->environment.newMaterial(material, tmp_idx)) { return false; }
			if(!core->environment.newEntity(tmp_idx, e_idx)) { return false; }
			if(!core->environment.newVertex(v0, tmp_idx)) { return false; }
			primitive.vertices.x = tmp_idx;
			if(!core->environment.newVertex(v1, tmp_idx)) { return false; }
			primitive.vertices.y = tmp_idx;
			if(!core->environment.entityAddPrimitive(e_idx, primitive)) { return false; }
			return true;
		}
		/// <summary>
		/// Auxiliary cuboid insert.
		/// </summary>
		bool addSphere(Core::Vertex v0, float radius, Core::Material material, std::uint32_t & e_idx)
		{
			std::uint32_t tmp_idx { 0U };
			Core::Primitive primitive
			{
				Core::Primitive::Types::Sphere, 0U, 0U, radius,
				glm::uvec4(0U, 0U, 0U, 0U)
			};
			if(!core->environment.newMaterial(material, tmp_idx)) { return false; }
			if(!core->environment.newEntity(tmp_idx, e_idx)) { return false; }
			if(!core->environment.newVertex(v0, tmp_idx)) { return false; }
			primitive.vertices.x = tmp_idx;
			if(!core->environment.entityAddPrimitive(e_idx, primitive)) { return false; }
			return true;
		}
		/// <summary>
		/// Auxiliary cuboid insert.
		/// </summary>
		bool addModel(std::string path, Core::Material material, std::uint32_t & e_idx)
		{
			std::uint32_t tmp_idx { 0U };
			if(!core->environment.newMaterial(material, tmp_idx)) { return false; }
			if(!core->environment.newEntity(tmp_idx, e_idx)) { return false; }
			if(!core->environment.entityLoadModel(e_idx, path)) { return false; }
			return true;
		}
	};

	// ------------------------------------------------------------------ //
	// Base tests.
	// ------------------------------------------------------------------ //
	TEST_F(CoreEnv, BuildCornellBox)
	{
		std::uint32_t e_idx { 0U };
		Core::Vertex cuboid_v0 { glm::vec3(-0.5f, -0.5f, -0.5f) };
		Core::Vertex cuboid_v1 { glm::vec3(0.5f, 0.5f, 0.5f) };
		Core::Vertex sphere_v0 { glm::vec3(0.0f, 0.0f, 0.0f) };

		// Floor
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.75f, 0.75f, 1.0f),
				Core::Material::Types::Diffuse, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(4.2f, 0.2f, 4.2f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.0f, -2.1f, 0.0f));
		}
		// Roof
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.75f, 0.75f, 1.0f),
				Core::Material::Types::Diffuse, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(4.2f, 0.2f, 4.2f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.0f, 2.1f, 0.0f));
		}
		// Right Wall
		{
			Core::Material material
			{
				glm::vec4(0.2f, 0.75f, 0.2f, 1.0f),
				Core::Material::Types::Diffuse, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(0.2f, 4.0f, 4.0f));
			core->environment.entityTranslate(e_idx, glm::vec3(-2.1f, 0.0f, 0.0f));
		}
		// Left Wall
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.2f, 0.2f, 1.0f),
				Core::Material::Types::Diffuse, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(0.2f, 4.0f, 4.0f));
			core->environment.entityTranslate(e_idx, glm::vec3(2.1f, 0.0f, 0.0f));
		}
		// End Wall
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.75f, 0.75f, 1.0f),
				Core::Material::Types::Diffuse, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(4.2f, 4.2f, 0.2f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.0f, 0.0f, -2.0f));
		}
		// Lamp
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.75f, 0.75f, 10.0f),
				Core::Material::Types::Emissive, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(0.5f, 0.025f, 0.5f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.0f, 1.95f, 0.0f));
		}
		// Lamp
		{
			Core::Material material
			{
				glm::vec4(0.75f, 0.75f, 0.75f, 0.0f),
				Core::Material::Types::Specular, 0.0f, 0.0f
			};
			ASSERT_TRUE(addCuboid(cuboid_v0, cuboid_v1, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(0.75f, 0.05f, 0.75f));
			core->environment.entityTranslate(e_idx, glm::vec3(0.0f, 1.95f, 0.0f));
		}
		// Big Sphere
		{
			Core::Material material
			{
				glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
				Core::Material::Types::Specular,
				0.0f, 0.15f
			};
			ASSERT_TRUE(addSphere(sphere_v0, 0.75f, material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(1.0f, 1.0f, 1.0f));
			core->environment.entityTranslate(e_idx, glm::vec3(-1.0f, -1.25f, -1.0f));
		}
		// Cube
		{
			Core::Material material
			{
				glm::vec4(0.5f, 0.75f, 0.75f, 0.3f),
				Core::Material::Types::Specular,
				1.3f, 0.0f
			};
			ASSERT_TRUE(addModel("models/cube.obj", material, e_idx));
			core->environment.entityScale(e_idx, glm::vec3(1.0f, 1.0f, 1.0f));
			core->environment.entityTranslate(e_idx, glm::vec3(1.0f, -1.5f, -0.75f));
			core->environment.entityRotate(e_idx, glm::vec3(0.0f, 0.6f, 0.0f));
		}
	}
	TEST_F(CoreEnv, PrimarySixtyFrameLoop)
	{
		{
			std::shared_lock<std::shared_mutex> scene_lock(core->environment.guard);
			{
				std::unique_lock<std::mutex> camera_lock(core->environment.scene->camera.guard);
				core->environment.scene->camera.data.look_from = glm::vec3(0.0f, 0.0f, 4.75f);
				core->environment.scene->camera.data.look_at = glm::vec3(0.0f, 0.0f, -0.825f);
				core->environment.scene->camera.data.v_up = glm::vec3(0.0f, 1.0f, 0.0f);
				core->environment.scene->camera.data.aperture = 0.25f;
				core->environment.scene->camera.data.focus = glm::length(
					core->environment.scene->camera.data.look_from -
					core->environment.scene->camera.data.look_at
				);
				core->environment.scene->camera.updated = true;
			}
		}
		core->run(10U, "../results.txt");
		ASSERT_TRUE(core->frame_counter >= 10U);
	}
	TEST_F(CoreEnv, SecondarySixtyFrameLoop)
	{
		core->run(60U, "../results.txt");
		ASSERT_TRUE(core->frame_counter >= 60U);
	}
	/*
	TEST_F(CoreEnv, InfLoop)
	{
		core->run();
	}
	*/
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


