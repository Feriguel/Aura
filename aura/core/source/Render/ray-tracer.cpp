// ========================================================================== //
// File : render.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include "ray-tracer.hpp"
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
#include <Aura/Core/Render/structures.hpp>
#include <Aura/Core/Utilities/thread_pool.hpp>
#include "framework.hpp"
#include "swapchain.hpp"
// Standard includes.
#include <array>
#include <cstdint>
#include <vector>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the ray tracer structure.
	/// </summary>
	RayTracer::RayTracer(
		vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
		vk::PhysicalDevice const & physical_device, vk::Device const & device,
		vk::SurfaceKHR const & surface, vk::Extent2D const & chain_base_extent,
		std::uint32_t const compute_family, std::uint32_t const transfer_family,
		std::uint32_t const present_family, ThreadPool & thread_pool,
		std::shared_mutex & scene_guard, Scene * const & scene) :
		VulkanSwapchain(dispatch, instance, physical_device, device,
			surface, chain_base_extent, std::vector<std::uint32_t>{compute_family, present_family}),
		scene_guard(scene_guard), scene(scene),
		compute_family(compute_family), transfer_family(transfer_family)
	{
		setUpResources(thread_pool);
		setUpPipelines(thread_pool);
	}
	/// <summary>
	/// Tears-down the ray tracer.
	/// </summary>
	RayTracer::~RayTracer() noexcept
	{
		tearDownPipelines();
		tearDownResources();
	}
	// ------------------------------------------------------------------ //
	// Recordings.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Records a ray gen operation.
	/// </summary>
	void RayTracer::recordRayGen(std::uint32_t const & width, std::uint32_t const & height,
		RandomOffset & push, vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, gen.pipeline, dispatch);
		std::array<vk::DescriptorSet, 2> sets { ray_launcher.set, chain_image.set };
		command.bindDescriptorSets(bind_point, gen.layout, 0U,
			2U, sets.data(), 0U, nullptr, dispatch);
		command.pushConstants(gen.layout, vk::ShaderStageFlagBits::eCompute,
			0U, sizeof(RandomOffset), reinterpret_cast<void *>(&push), dispatch);
		command.dispatch(width / raygen_gsize[0], height / raygen_gsize[1],
			raygen_gsize[2], dispatch);
	}

	// ------------------------------------------------------------------ //
	// Resource updates.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Updates the ray launcher using the camera in the scene.
	/// </summary>
	void RayTracer::updateRayLauncher(std::uint32_t const & width, std::uint32_t const & height)
	{
		RayLauncher launcher {};
		Camera camera {};
		bool has_update = false;
		{
			std::shared_lock<std::shared_mutex> lock(scene_guard);
			std::unique_lock<std::mutex> camera_lock(scene->camera.guard);
			if(scene->camera.updated)
			{
				camera = scene->camera.data;
				has_update = true;
				scene->camera.updated = false;
			}
		}
		if(!has_update) { return; }
		{
			// Pre-known values.
			constexpr gpu_real pi = static_cast<gpu_real>(3.141592653589793);
			constexpr gpu_real to_radians = pi / static_cast<gpu_real>(180.0);
			constexpr gpu_real two = static_cast<gpu_real>(2.0);
			// Update camera points and vectors.
			gpu_vec3 const lookfrom = gpu_vec3(
				camera.transform.translation * camera.transform.rotation * camera.transform.scaling
				* gpu_vec4(camera.look_from, 1.0));
			gpu_vec3 const lookat = gpu_vec3(
				camera.transform.translation * camera.transform.rotation * camera.transform.scaling
				* gpu_vec4(camera.look_at, 1.0));
			gpu_vec3 const vup = gpu_vec3(
				camera.transform.translation * camera.transform.rotation * camera.transform.scaling
				* gpu_vec4(camera.v_up, 1.0));
			// Calculate helper values.
			gpu_real const aspect = static_cast<gpu_real>(width) / static_cast<gpu_real>(height);
			gpu_real const theta = camera.v_fov * to_radians;
			gpu_real const half_height = tan(theta / two);
			gpu_real const half_width = aspect * half_height;
			// Calculate ray launcher specification.
			launcher.lens_radius = camera.aperture / two;
			launcher.origin = lookfrom;
			launcher.w = glm::normalize(lookfrom - lookat);
			launcher.u = -glm::normalize(glm::cross(vup, launcher.w));
			launcher.v = glm::cross(launcher.w, launcher.u);
			launcher.vertical = half_height * camera.focus * -launcher.v;
			launcher.horizontal = half_width * camera.focus * -launcher.u;
			launcher.corner = launcher.origin + launcher.vertical + launcher.horizontal - camera.focus * launcher.w;
			launcher.vertical *= 2;
			launcher.horizontal *= -2;
		}
		void * mem { nullptr };
		vk::Result result { device.mapMemory(ray_launcher.memories[0],
			ray_launcher.buffers[0].offset,ray_launcher.buffers[0].range, {}, &mem, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "RayLauncher memory map"); }
		if(mem)
		{
			std::memcpy(mem, &launcher, ray_launcher.buffers[0].range);
			device.unmapMemory(ray_launcher.memories[0], dispatch);
		}
	}

	// ------------------------------------------------------------------ //
	// Resources.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets up the descriptor pool and bulk sets-up all the resources needed.
	/// </summary>
	void RayTracer::setUpResources(ThreadPool & thread_pool)
	{
		setUpDescriptorPool();

		auto t1 = thread_pool.enqueue([&] { setUpChainImage(); });
		auto t2 = thread_pool.enqueue([&] { setUpRayLauncher(); });

		if(!t1.valid()) { throw std::future_error(std::future_errc::no_state); }
		if(!t2.valid()) { throw std::future_error(std::future_errc::no_state); }
		t1.wait();
		t2.wait();

		allocateAllDescriptorSets();
		updateRayLauncherSet();
	}
	/// <summary>
	/// Bulk tears-down all set-up resources, as well as, the descriptor pool.
	/// </summary>
	void RayTracer::tearDownResources()
	{
		tearDownChainImage();
		tearDownRayLauncher();
		tearDownDescriptorPool();
	}
	/// <summary>
	/// Allocates all descriptor sets in one go.
	/// </summary>
	void RayTracer::allocateAllDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts {};
		std::vector<vk::DescriptorSet> sets {};

		layouts.emplace_back(chain_image.set_layout);
		layouts.emplace_back(ray_launcher.set_layout);

		std::size_t n_sets { layouts.size() };
		sets.resize(n_sets);
		allocateDescriptorSets(pool, static_cast<std::uint32_t>(n_sets), layouts.data(), sets.data());

		chain_image.set = sets[0];
		ray_launcher.set = sets[1];
	}
	/// <summary>
	/// Sets up the descriptor pool for all resources.
	/// </summary>
	void RayTracer::setUpDescriptorPool()
	{
		constexpr std::uint32_t n_sets { 4U };
		constexpr std::uint32_t n_sizes { 2U };
		std::array<vk::DescriptorPoolSize, n_sizes> sizes {
			// - Descriptor type and count.
			vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, 1U },
			vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, 1U } };
		createDescriptorPool({}, n_sets, n_sizes, sizes.data(), pool);
	}
	/// <summary>
	/// Tears-down the descriptor pool.
	/// </summary>
	void RayTracer::tearDownDescriptorPool()
	{
		destroyDescriptorPool(pool);
	}
	/// <summary>
	/// Prepares a chain image layout.
	/// </summary>
	void RayTracer::setUpChainImage()
	{
		std::array<vk::DescriptorSetLayoutBinding, 1U> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageImage, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, 1U, binds.data(), chain_image.set_layout);
	}
	/// <summary>
	/// Updates the chain image according to the frame index.
	/// </summary>
	void RayTracer::updateChainImageSet(std::uint32_t frame_index)
	{
		std::array<vk::DescriptorImageInfo, 1U> const images {
			// - Sampler, view, layout.
			vk::DescriptorImageInfo {nullptr, chain_views[frame_index], vk::ImageLayout::eGeneral}
		};
		std::array<vk::WriteDescriptorSet, 1U> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{chain_image.set, 0U, 0U, 1U,
				vk::DescriptorType::eStorageImage, &images[0], nullptr, nullptr}
		};
		device.updateDescriptorSets(1U, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys chain image layout.
	/// </summary>
	void RayTracer::tearDownChainImage()
	{
		destroyDescriptorSetLayout(chain_image.set_layout);
	}
	/// <summary>
	/// Prepares ray launcher layout, buffer and its memory.
	/// </summary>
	void RayTracer::setUpRayLauncher()
	{
		constexpr std::uint32_t n_buffers { 1U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eUniformBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), ray_launcher.set_layout);

		ray_launcher.buffers.resize(n_buffers);
		ray_launcher.buffers[0].range = static_cast<vk::DeviceSize>(sizeof(RayLauncher));
		createBuffer({}, ray_launcher.buffers[0].range, vk::BufferUsageFlagBits::eUniformBuffer,
			1U, &compute_family, ray_launcher.buffers[0].buffer);
		vk::MemoryRequirements mem_launcher {};
		device.getBufferMemoryRequirements(ray_launcher.buffers[0].buffer, &mem_launcher, dispatch);

		ray_launcher.memories.resize(1U);
		allocateMemory(mem_launcher,
			vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			ray_launcher.memories[0]);
		device.bindBufferMemory(ray_launcher.buffers[0].buffer, ray_launcher.memories[0],
			ray_launcher.buffers[0].offset, dispatch);
	}
	/// <summary>
	/// Writes ray launcher sets.
	/// </summary>
	void RayTracer::updateRayLauncherSet()
	{
		std::array<vk::WriteDescriptorSet, 1U> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{ray_launcher.set, 0U, 0U, 1U,
				vk::DescriptorType::eUniformBuffer, nullptr, &ray_launcher.buffers[0], nullptr}
		};
		device.updateDescriptorSets(1U, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys ray launcher layout, buffer and its respective memory.
	/// </summary>
	void RayTracer::tearDownRayLauncher()
	{
		std::size_t n { ray_launcher.buffers.size() };
		for(std::uint32_t i { 0U }; i < n; ++i)
		{ destroyBuffer(ray_launcher.buffers[i].buffer); }

		n = ray_launcher.memories.size();
		for(std::uint32_t i { 0U }; i < n; ++i)
		{ freeMemory(ray_launcher.memories[i]); }

		destroyDescriptorSetLayout(ray_launcher.set_layout);
	}

	// ------------------------------------------------------------------ //
	// Pipelines.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Bulk sets-up all pipelines.
	/// </summary>
	void RayTracer::setUpPipelines(ThreadPool & thread_pool)
	{
		auto t1 = thread_pool.enqueue([&] { setUpGenPipeline(); });

		if(!t1.valid()) { throw std::future_error(std::future_errc::no_state); }
		t1.wait();
	}
	/// <summary>
	/// Bulk tears-down all set-up pipelines.
	/// </summary>
	void RayTracer::tearDownPipelines()
	{
		tearDownGenPipeline();
	}
	/// <summary>
	/// Prepares the ray generation layout and shader module.
	/// </summary>
	void RayTracer::setUpGenPipeline()
	{
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 2U> const set_layouts {
			ray_launcher.set_layout, chain_image.set_layout };
		vk::PushConstantRange const push {
			vk::ShaderStageFlagBits::eCompute, 0U, sizeof(RandomOffset) };
		createPipelineLayout({}, 2U, set_layouts.data(), 1U, &push, gen.layout);

		auto path = std::string(shader_folder);
		path += "ray-gen.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			gen.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &gen.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the ray generation pipeline, layout 
	/// </summary>
	void RayTracer::tearDownGenPipeline()
	{
		destroyPipelineLayout(gen.layout);
		destroyPipeline(gen.pipeline);
	}
}
