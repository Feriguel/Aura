// ========================================================================== //
// File : render.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include "ray-tracer.hpp"
// Internal includes.
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
#include <utility>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)
#include <GLFW/glfw3.h>
#pragma warning(disable : 26812)
#include <glm/glm.hpp>
#pragma warning(default : 26812)

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
		std::uint32_t const width, std::uint32_t const height,
		std::shared_mutex & scene_guard, Scene * const & scene) :
		VulkanSwapchain(dispatch, instance, physical_device, device,
			surface, chain_base_extent, std::vector<std::uint32_t>{compute_family, present_family}),
		width(width), height(height),
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
	/// Records a pre-processing operation.
	/// </summary>
	void RayTracer::recordPreProcess(vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 2U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets { render_settings.set, rays_state.set };

		command.bindPipeline(bind_point, pre_process.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, pre_process.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		std::uint32_t x = (width + pre_gsize[0U] - 1) / pre_gsize[0U];
		std::uint32_t y = (height + pre_gsize[1U] - 1) / pre_gsize[1U];
		std::uint32_t z = (1U + pre_gsize[2U] - 1) / pre_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}
	/// <summary>
	/// Records a vertex input transform operation.
	/// </summary>
	void RayTracer::recordVertex(vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 2U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets {
			render_settings.set, scene_info.set };

		command.bindPipeline(bind_point, vertex.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, vertex.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		std::uint32_t x = (n_primitives + vertex_gsize[0U] - 1) / vertex_gsize[0U];
		std::uint32_t y = (1U + vertex_gsize[1U] - 1) / vertex_gsize[1U];
		std::uint32_t z = (1U + vertex_gsize[2U] - 1) / vertex_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}
	/// <summary>
	/// Records a ray gen operation.
	/// </summary>
	void RayTracer::recordRayGen(RandomSeed & push, vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets {
			render_settings.set, ray_launcher.set, rays_state.set };

		command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
			{}, 0, nullptr, 0, nullptr, 0, nullptr, dispatch);
		command.bindPipeline(bind_point, gen.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, gen.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		command.pushConstants(gen.layout, vk::ShaderStageFlagBits::eCompute,
			0U, sizeof(RandomSeed), reinterpret_cast<void *>(&push), dispatch);
		std::uint32_t x = (width + gen_gsize[0U] - 1) / gen_gsize[0U];
		std::uint32_t y = (height + gen_gsize[1U] - 1) / gen_gsize[1U];
		std::uint32_t z = (1U + gen_gsize[2U] - 1) / gen_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}
	/// <summary>
	/// Records a intersect operation.
	/// </summary>
	void RayTracer::recordIntersect(vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets {
			render_settings.set, rays_state.set, scene_info.set };

		command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
			{}, 0, nullptr, 0, nullptr, 0, nullptr, dispatch);
		command.bindPipeline(bind_point, intersect.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, intersect.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		std::uint32_t x = (width + intersect_gsize[0U] - 1) / intersect_gsize[0U];
		std::uint32_t y = (height + intersect_gsize[1U] - 1) / intersect_gsize[1U];
		std::uint32_t z = (1U + intersect_gsize[2U] - 1) / intersect_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}
	/// <summary>
	/// Records a scatter operation.
	/// </summary>
	void RayTracer::recordColourAndScatter(RandomPointInCircleAndSeed & push, vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets {
			render_settings.set, rays_state.set, scene_info.set };

		command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
			{}, 0, nullptr, 0, nullptr, 0, nullptr, dispatch);
		command.bindPipeline(bind_point, colour_and_scatter.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, colour_and_scatter.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		command.pushConstants(colour_and_scatter.layout, vk::ShaderStageFlagBits::eCompute,
			0U, sizeof(RandomPointInCircleAndSeed), reinterpret_cast<void *>(&push), dispatch);
		std::uint32_t x = (width + colour_and_scatter_gsize[0U] - 1) / colour_and_scatter_gsize[0U];
		std::uint32_t y = (height + colour_and_scatter_gsize[1U] - 1) / colour_and_scatter_gsize[1U];
		std::uint32_t z = (1U + colour_and_scatter_gsize[2U] - 1) / colour_and_scatter_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}
	/// <summary>
	/// Records a post-processing operation.
	/// </summary>
	void RayTracer::recordPostProcess(vk::CommandBuffer const & command) const
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineBindPoint const bind_point { vk::PipelineBindPoint::eCompute };
		std::array<vk::DescriptorSet, n_sets> const sets {
			render_settings.set, rays_state.set, chain_image.set };

		command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
			{}, 0, nullptr, 0, nullptr, 0, nullptr, dispatch);
		command.bindPipeline(bind_point, post_process.pipeline, dispatch);
		command.bindDescriptorSets(bind_point, post_process.layout, 0U,
			n_sets, sets.data(), 0U, nullptr, dispatch);
		std::uint32_t x = (width + post_gsize[0U] - 1) / post_gsize[0U];
		std::uint32_t y = (height + post_gsize[1U] - 1) / post_gsize[1U];
		std::uint32_t z = (1U + post_gsize[2U] - 1) / post_gsize[2U];
		command.dispatch(x, y, z, dispatch);
	}

	// ------------------------------------------------------------------ //
	// Resource updates.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Updates the ray launcher using the camera in the scene.
	/// </summary>
	void RayTracer::updateRenderSettings(float const t_min, float const t_max,
		std::uint32_t const n_samples, std::uint32_t const n_bounces)
	{
		{
			std::shared_lock<std::shared_mutex> lock(scene_guard);
			std::unique_lock<std::mutex> primitives_lock(scene->primitives.guard);
			n_primitives = static_cast<std::uint32_t>(scene->primitives.data.size());
		}

		RenderSettings settings {};
		settings.width = width;
		settings.height = height;
		settings.t_min = t_min;
		settings.t_max = t_max;
		settings.n_samples = n_samples;
		settings.n_bounces = n_bounces;
		settings.n_primitives = n_primitives;

		void * mem { nullptr };
		vk::Result result { device.mapMemory(render_settings.memories[0U],
			render_settings.buffers[0U].offset, render_settings.buffers[0U].range, {}, &mem, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "RenderSettings memory map"); }
		if(mem)
		{
			std::memcpy(mem, &settings, render_settings.buffers[0U].range);
			device.unmapMemory(render_settings.memories[0U], dispatch);
		}
	}
	/// <summary>
	/// Updates the ray launcher using the camera in the scene.
	/// </summary>
	bool RayTracer::updateRayLauncher()
	{
		RayLauncher launcher {};
		Camera camera {};
		{
			std::shared_lock<std::shared_mutex> lock(scene_guard);
			std::unique_lock<std::mutex> camera_lock(scene->camera.guard);
			if(!scene->camera.updated)
			{
				return false;
			}
			camera = scene->camera.data;
			scene->camera.updated = false;
		}
		{
			// Pre-known values.
			constexpr float pi = static_cast<float>(3.141592653589793);
			constexpr float to_radians = pi / 180.0f;
			// Update camera points and vectors.
			glm::mat4 const transform = camera.transform.translation *
				camera.transform.rotation * camera.transform.scaling;
			glm::vec3 const lookfrom = glm::vec3(transform * glm::vec4(camera.look_from, 1.0f));
			glm::vec3 const lookat = glm::vec3(transform * glm::vec4(camera.look_at, 1.0f));
			glm::vec3 const vup = glm::vec3(transform * glm::vec4(camera.v_up, 1.0f));
			// Calculate helper values.
			float const aspect = static_cast<float>(width) / static_cast<float>(height);
			float const theta = camera.v_fov * to_radians;
			float const half_height = tan(theta / 2.0f);
			float const half_width = aspect * half_height;
			// Calculate ray launcher specification.
			launcher.lens_radius = camera.aperture / 2.0f;
			launcher.origin = lookfrom;
			launcher.w = glm::normalize(lookfrom - lookat);
			launcher.u = glm::normalize(glm::cross(vup, launcher.w));
			launcher.v = glm::cross(launcher.w, launcher.u);
			launcher.vertical = half_height * camera.focus * launcher.v;
			launcher.horizontal = half_width * camera.focus * launcher.u;
			launcher.corner = launcher.origin + launcher.vertical - launcher.horizontal - camera.focus * launcher.w;
			launcher.vertical *= 2.0f;
			launcher.horizontal *= 2.0f;
		}
		void * mem { nullptr };
		vk::Result result { device.mapMemory(ray_launcher.memories[0U],
			ray_launcher.buffers[0U].offset,ray_launcher.buffers[0U].range, {}, &mem, dispatch) };
		if(result != vk::Result::eSuccess)
		{ vk::throwResultException(result, "RayLauncher memory map"); }
		if(mem)
		{
			std::memcpy(mem, &launcher, ray_launcher.buffers[0U].range);
			device.unmapMemory(ray_launcher.memories[0U], dispatch);
		}
		return true;
	}
	/// <summary>
	/// Updates scene vertices.
	/// </summary>
	bool RayTracer::updateScene()
	{
		bool update = false;
		std::shared_lock<std::shared_mutex> scene_lock(scene_guard);
		{
			std::unique_lock<std::mutex> vertices_lock(scene->vertices.guard);
			if(scene->vertices.updated)
			{
				void * mem { nullptr };
				vk::Result result { device.mapMemory(scene_info.memories[0U],
					scene_info.buffers[0U].offset, scene_info.buffers[0U].range, {}, &mem, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "Vertices memory map"); }
				if(mem)
				{
					std::memcpy(mem, scene->vertices.data.data(), scene->vertices.data.size() * sizeof(Vertex));
					device.unmapMemory(scene_info.memories[0U], dispatch);
				}
				scene->vertices.updated = false;
				update = true;
			}
		}
		{
			std::unique_lock<std::mutex> transforms_lock(scene->transforms.guard);
			if(scene->transforms.updated)
			{
				void * mem { nullptr };
				vk::Result result { device.mapMemory(scene_info.memories[0U],
					scene_info.buffers[1U].offset, scene_info.buffers[1U].range, {}, &mem, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "Transforms memory map"); }
				if(mem)
				{
					std::memcpy(mem, scene->transforms.data.data(), scene->transforms.data.size() * sizeof(Transform));
					device.unmapMemory(scene_info.memories[0U], dispatch);
				}
				scene->transforms.updated = false;
				update = true;
			}
		}
		{
			std::unique_lock<std::mutex> materials_lock(scene->materials.guard);
			if(scene->materials.updated)
			{
				void * mem { nullptr };
				vk::Result result { device.mapMemory(scene_info.memories[0U],
					scene_info.buffers[2U].offset,scene_info.buffers[2U].range, {}, &mem, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "Materials memory map"); }
				if(mem)
				{
					std::memcpy(mem, scene->materials.data.data(), scene->materials.data.size() * sizeof(Material));
					device.unmapMemory(scene_info.memories[0U], dispatch);
				}
				scene->materials.updated = false;
				update = true;
			}
		}
		{
			std::unique_lock<std::mutex> primitives_lock(scene->primitives.guard);
			if(scene->primitives.updated)
			{
				void * mem { nullptr };
				vk::Result result { device.mapMemory(scene_info.memories[0U],
					scene_info.buffers[3U].offset,scene_info.buffers[3U].range, {}, &mem, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "Primitives memory map"); }
				if(mem)
				{
					std::memcpy(mem, scene->primitives.data.data(), n_primitives * sizeof(Primitive));
					device.unmapMemory(scene_info.memories[0U], dispatch);
				}
				scene->primitives.updated = false;
				update = true;
			}
		}
		return update;
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

		std::array<std::future<void>, 4U> const jobs {
			thread_pool.enqueue([&] { setUpRenderSettings(); }),
			thread_pool.enqueue([&] { setUpRayLauncher(); }),
			thread_pool.enqueue([&] { setUpRaysState(); }),
			thread_pool.enqueue([&] { setUpSceneInfo(); })
		};

		for(std::size_t i { 0U }; i < jobs.size(); ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
		allocateAllDescriptorSets();

		updateRenderSettingsSet();
		updateRayLauncherSet();
		updateRaysState();
		updateSceneInfo();
	}
	/// <summary>
	/// Bulk tears-down all set-up resources, as well as, the descriptor pool.
	/// </summary>
	void RayTracer::tearDownResources()
	{
		tearDownRenderSettings();
		tearDownRaysState();
		tearDownRayLauncher();
		tearDownSceneInfo();
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
		layouts.emplace_back(render_settings.set_layout);
		layouts.emplace_back(ray_launcher.set_layout);
		layouts.emplace_back(rays_state.set_layout);
		layouts.emplace_back(scene_info.set_layout);

		sets.resize(layouts.size());
		allocateDescriptorSets(pool, static_cast<std::uint32_t>(layouts.size()), layouts.data(), sets.data());

		chain_image.set = sets[0U];
		render_settings.set = sets[1U];
		ray_launcher.set = sets[2U];
		rays_state.set = sets[3U];
		scene_info.set = sets[4U];
	}
	/// <summary>
	/// Sets up the descriptor pool for all resources.
	/// </summary>
	void RayTracer::setUpDescriptorPool()
	{
		constexpr std::uint32_t n_sets { 5U };
		constexpr std::uint32_t n_sizes { 3U };
		std::array<vk::DescriptorPoolSize, n_sizes> sizes {
			// - Descriptor type and count.
			vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, 1U },
			vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, 8U },
			vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, 2U } };
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
	/// Prepares render settings layout, buffers and memory.
	/// </summary>
	void RayTracer::setUpRenderSettings()
	{
		constexpr std::uint32_t n_buffers { 1U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eUniformBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), render_settings.set_layout);

		vk::DeviceSize const settings_size { static_cast<vk::DeviceSize>(sizeof(RenderSettings)) };
		vk::Buffer buffer {};
		createBuffer({}, settings_size, vk::BufferUsageFlagBits::eUniformBuffer, 1U, &compute_family, buffer);
		vk::MemoryRequirements mem {};
		device.getBufferMemoryRequirements(buffer, &mem, dispatch);

		render_settings.buffers.resize(n_buffers);
		render_settings.buffers[0U].buffer = buffer;
		render_settings.buffers[0U].range = settings_size;
		render_settings.buffers[0U].offset = 0U;

		vk::MemoryPropertyFlags const required {
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
		std::uint32_t type_index { 0U };
		if(!findMemoryType(mem.memoryTypeBits, required, type_index))
		{
			throw std::exception("No memory with required properties. [Render Settings]");
		}

		render_settings.memories.resize(1U);
		allocateMemory(mem.size, type_index, render_settings.memories[0U]);
		device.bindBufferMemory(render_settings.buffers[0U].buffer, render_settings.memories[0U],
			render_settings.buffers[0U].offset, dispatch);
	}
	/// <summary>
	/// Update render settings set. This isn't mutable, and should only be
	/// used once.
	/// </summary>
	void RayTracer::updateRenderSettingsSet()
	{
		std::array<vk::WriteDescriptorSet, 1U> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{render_settings.set, 0U, 0U, 1U,
				vk::DescriptorType::eUniformBuffer, nullptr, &render_settings.buffers[0], nullptr}
		};
		device.updateDescriptorSets(1U, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys the render settings layout, buffers and memory.
	/// </summary>
	void RayTracer::tearDownRenderSettings()
	{
		destroyBuffer(render_settings.buffers[0U].buffer);
		freeMemory(render_settings.memories[0U]);
		destroyDescriptorSetLayout(render_settings.set_layout);
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

		vk::DeviceSize const launcher_size { static_cast<vk::DeviceSize>(sizeof(RayLauncher)) };
		vk::Buffer buffer {};
		createBuffer({}, launcher_size, vk::BufferUsageFlagBits::eUniformBuffer, 1U, &compute_family, buffer);
		vk::MemoryRequirements mem {};
		device.getBufferMemoryRequirements(buffer, &mem, dispatch);

		ray_launcher.buffers.resize(n_buffers);
		ray_launcher.buffers[0U].buffer = buffer;
		ray_launcher.buffers[0U].range = launcher_size;
		ray_launcher.buffers[0U].offset = 0U;

		vk::MemoryPropertyFlags const required {
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
		std::uint32_t type_index { 0U };
		if(!findMemoryType(mem.memoryTypeBits, required, type_index))
		{
			throw std::exception("No memory with required properties. [Ray Launcher]");
		}

		ray_launcher.memories.resize(1U);
		allocateMemory(mem.size, type_index, ray_launcher.memories[0U]);
		device.bindBufferMemory(ray_launcher.buffers[0U].buffer, ray_launcher.memories[0U],
			ray_launcher.buffers[0U].offset, dispatch);
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
		destroyBuffer(ray_launcher.buffers[0U].buffer);
		freeMemory(ray_launcher.memories[0U]);
		destroyDescriptorSetLayout(ray_launcher.set_layout);
	}
	/// <summary>
	/// Prepares rays and hits layout, buffers and memory.
	/// </summary>
	void RayTracer::setUpRaysState()
	{
		constexpr std::uint32_t n_buffers { 3U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 1U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 2U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), rays_state.set_layout);

		std::array<vk::DeviceSize, n_buffers> sizes {
			static_cast<vk::DeviceSize>(sizeof(Ray) * width * height),
			static_cast<vk::DeviceSize>(sizeof(Hit) * width * height),
			static_cast<vk::DeviceSize>(sizeof(Pixel) * width * height)
		};
		rays_state.buffers.resize(n_buffers);

		vk::MemoryPropertyFlags const required { vk::MemoryPropertyFlagBits::eDeviceLocal };
		vk::DeviceSize total_size { 0U };
		std::uint32_t type_bits { 0U };
		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			vk::MemoryRequirements mem {};
			std::uint32_t mem_type { 0U };

			createBuffer({}, sizes[i], vk::BufferUsageFlagBits::eStorageBuffer, 1U, &compute_family, rays_state.buffers[i].buffer);
			device.getBufferMemoryRequirements(rays_state.buffers[i].buffer, &mem, dispatch);
			if(!findMemoryType(mem.memoryTypeBits, required, mem_type))
			{
				throw std::exception("No memory with required properties. [Rays state]");
			}
			if(i == 0)
			{
				type_bits = mem_type;
			}
			else if(mem_type != type_bits)
			{
				throw std::exception("Memory type bits are different, case not implemented. [Rays state]");
			}
			rays_state.buffers[i].offset = total_size;
			rays_state.buffers[i].range = sizes[i];
			total_size += mem.size;
		}

		rays_state.memories.resize(1U);
		allocateMemory(total_size, type_bits, rays_state.memories[0U]);
		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			device.bindBufferMemory(rays_state.buffers[i].buffer, rays_state.memories[0U],
				rays_state.buffers[i].offset, dispatch);
		}
	}
	/// <summary>
	/// Update rays and hits set. This isn't mutable, and should only be
	/// used once.
	/// </summary>
	void RayTracer::updateRaysState()
	{
		constexpr std::uint32_t n_buffers { 3U };

		std::array<vk::DescriptorBufferInfo, n_buffers> buffers {
			rays_state.buffers[0U], rays_state.buffers[1U], rays_state.buffers[2U] };
		buffers[0U].offset = 0U;
		buffers[1U].offset = 0U;
		buffers[2U].offset = 0U;
		std::array<vk::WriteDescriptorSet, n_buffers> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{rays_state.set, 0U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[0U], nullptr},
			vk::WriteDescriptorSet{rays_state.set, 1U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[1U], nullptr},
			vk::WriteDescriptorSet{rays_state.set, 2U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[2U], nullptr}
		};
		device.updateDescriptorSets(n_buffers, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys the rays and hits layout, buffers and memory.
	/// </summary>
	void RayTracer::tearDownRaysState()
	{
		constexpr std::uint32_t n_buffers { 3U };

		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			destroyBuffer(rays_state.buffers[i].buffer);
		}
		freeMemory(rays_state.memories[0U]);
		destroyDescriptorSetLayout(rays_state.set_layout);
	}
	/// <summary>
	/// Prepares scene info layout, buffers and memory.
	/// </summary>
	void RayTracer::setUpSceneInfo()
	{
		constexpr std::uint32_t n_buffers { 5U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 1U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 2U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 3U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 4U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), scene_info.set_layout);

		std::array<vk::DeviceSize, n_buffers> sizes {
			static_cast<vk::DeviceSize>(sizeof(Vertex) * EnvLimits::limit_vertices),
			static_cast<vk::DeviceSize>(sizeof(Transform) * EnvLimits::limit_entities),
			static_cast<vk::DeviceSize>(sizeof(Material) * EnvLimits::limit_materials),
			static_cast<vk::DeviceSize>(sizeof(Primitive) * EnvLimits::limit_primitives),
			static_cast<vk::DeviceSize>(sizeof(glm::vec4) * EnvLimits::limit_primitives * 3U)
		};
		scene_info.buffers.resize(n_buffers);

		vk::MemoryPropertyFlags const required {
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
		vk::DeviceSize total_size { 0U };
		std::uint32_t type_bits { 0U };
		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			vk::MemoryRequirements mem {};
			std::uint32_t mem_type { 0U };

			createBuffer({}, sizes[i], vk::BufferUsageFlagBits::eStorageBuffer, 1U, &compute_family, scene_info.buffers[i].buffer);
			device.getBufferMemoryRequirements(scene_info.buffers[i].buffer, &mem, dispatch);
			if(!findMemoryType(mem.memoryTypeBits, required, mem_type))
			{
				throw std::exception("No memory with required properties. [Rays state]");
			}
			if(i == 0)
			{
				type_bits = mem_type;
			}
			else if(mem_type != type_bits)
			{
				throw std::exception("Memory type bits are different, case not implemented. [Rays state]");
			}
			scene_info.buffers[i].offset = total_size;
			scene_info.buffers[i].range = sizes[i];
			total_size += mem.size;
		}

		scene_info.memories.resize(1U);
		allocateMemory(total_size, type_bits, scene_info.memories[0U]);
		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			device.bindBufferMemory(scene_info.buffers[i].buffer, scene_info.memories[0U],
				scene_info.buffers[i].offset, dispatch);
		}
	}
	/// <summary>
	/// Update scene info set. This isn't mutable, and should only be
	/// used once.
	/// </summary>
	void RayTracer::updateSceneInfo()
	{
		constexpr std::uint32_t n_buffers { 5U };

		std::array<vk::DescriptorBufferInfo, n_buffers> buffers {
			scene_info.buffers[0U], scene_info.buffers[1U], scene_info.buffers[2U],
			scene_info.buffers[3U], scene_info.buffers[4U], };
		buffers[0U].offset = 0U;
		buffers[1U].offset = 0U;
		buffers[2U].offset = 0U;
		buffers[3U].offset = 0U;
		buffers[4U].offset = 0U;
		std::array<vk::WriteDescriptorSet, n_buffers> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{scene_info.set, 0U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[0U], nullptr},
			vk::WriteDescriptorSet{scene_info.set, 1U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[1U], nullptr},
			vk::WriteDescriptorSet{scene_info.set, 2U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[2U], nullptr},
			vk::WriteDescriptorSet{scene_info.set, 3U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[3U], nullptr},
			vk::WriteDescriptorSet{scene_info.set, 4U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &buffers[4U], nullptr}
		};
		device.updateDescriptorSets(n_buffers, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys the scene info layout, buffers and memory.
	/// </summary>
	void RayTracer::tearDownSceneInfo()
	{
		constexpr std::uint32_t n_buffers { 5U };

		for(std::size_t i { 0U }; i < n_buffers; ++i)
		{
			destroyBuffer(scene_info.buffers[i].buffer);
		}
		freeMemory(scene_info.memories[0U]);
		destroyDescriptorSetLayout(scene_info.set_layout);
	}

	// ------------------------------------------------------------------ //
	// Pipelines.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Bulk sets-up all pipelines.
	/// </summary>
	void RayTracer::setUpPipelines(ThreadPool & thread_pool)
	{
		constexpr std::size_t n_jobs { 6U };

		std::array<std::future<void>, n_jobs> const jobs {
			thread_pool.enqueue([&] { setUpPreProcessPipeline(); }),
			thread_pool.enqueue([&] { setUpVertexPipeline(); }),
			thread_pool.enqueue([&] { setUpGenPipeline(); }),
			thread_pool.enqueue([&] { setUpIntersectPipeline(); }),
			thread_pool.enqueue([&] { setUpColourScatterPipeline(); }),
			thread_pool.enqueue([&] { setUpPostProcessPipeline(); })
		};

		for(std::size_t i { 0U }; i < n_jobs; ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
	}
	/// <summary>
	/// Bulk tears-down all set-up pipelines.
	/// </summary>
	void RayTracer::tearDownPipelines()
	{
		tearDownPostProcessPipeline();
		tearDownColourScatterPipeline();
		tearDownIntersectPipeline();
		tearDownGenPipeline();
		tearDownVertexPipeline();
		tearDownPreProcessPipeline();
	}
	/// <summary>
	/// Prepares the pre-processing layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpPreProcessPipeline()
	{
		constexpr std::uint32_t n_sets { 2U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts { 
			render_settings.set_layout, rays_state.set_layout };
		createPipelineLayout({}, n_sets, set_layouts.data(), 0U, nullptr, pre_process.layout);

		auto path = std::string(shader_folder);
		path += "pre-process.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			post_process.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &pre_process.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the pre-processing pipeline, layout 
	/// </summary>
	void RayTracer::tearDownPreProcessPipeline()
	{
		destroyPipelineLayout(pre_process.layout);
		destroyPipeline(pre_process.pipeline);
	}
	/// <summary>
	/// Prepares the vertex layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpVertexPipeline()
	{
		constexpr std::uint32_t n_sets { 2U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts {
			render_settings.set_layout, scene_info.set_layout };
		createPipelineLayout({}, n_sets, set_layouts.data(), 0U, nullptr, vertex.layout);

		auto path = std::string(shader_folder);
		path += "vertex.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			vertex.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &vertex.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the vertex pipeline, layout 
	/// </summary>
	void RayTracer::tearDownVertexPipeline()
	{
		destroyPipelineLayout(vertex.layout);
		destroyPipeline(vertex.pipeline);
	}
	/// <summary>
	/// Prepares the ray generation layout and shader module.
	/// </summary>
	void RayTracer::setUpGenPipeline()
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts {
			render_settings.set_layout, ray_launcher.set_layout, rays_state.set_layout };
		vk::PushConstantRange const push {
			vk::ShaderStageFlagBits::eCompute, 0U, sizeof(RandomSeed) };
		createPipelineLayout({}, n_sets, set_layouts.data(), 1U, &push, gen.layout);

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
	/// <summary>
	/// Prepares the intersect layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpIntersectPipeline()
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts {
			render_settings.set_layout, rays_state.set_layout, scene_info.set_layout };
		createPipelineLayout({}, n_sets, set_layouts.data(), 0U, nullptr, intersect.layout);

		auto path = std::string(shader_folder);
		path += "intersect.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			intersect.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &intersect.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the intersect pipeline, layout 
	/// </summary>
	void RayTracer::tearDownIntersectPipeline()
	{
		destroyPipelineLayout(intersect.layout);
		destroyPipeline(intersect.pipeline);
	}
	/// <summary>
	/// Prepares the scatter layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpColourScatterPipeline()
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts {
			render_settings.set_layout, rays_state.set_layout, scene_info.set_layout };
		vk::PushConstantRange const push {
			vk::ShaderStageFlagBits::eCompute, 0U, sizeof(RandomPointInCircleAndSeed) };
		createPipelineLayout({}, n_sets, set_layouts.data(), 1U, &push, colour_and_scatter.layout);

		auto path = std::string(shader_folder);
		path += "colour_and_scatter.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			colour_and_scatter.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &colour_and_scatter.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the scatter pipeline, layout 
	/// </summary>
	void RayTracer::tearDownColourScatterPipeline()
	{
		destroyPipelineLayout(colour_and_scatter.layout);
		destroyPipeline(colour_and_scatter.pipeline);
	}
	/// <summary>
	/// Prepares the post-processing layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpPostProcessPipeline()
	{
		constexpr std::uint32_t n_sets { 3U };
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, n_sets> const set_layouts {
			render_settings.set_layout, rays_state.set_layout, chain_image.set_layout };
		createPipelineLayout({}, n_sets, set_layouts.data(), 0U, nullptr, post_process.layout);

		auto path = std::string(shader_folder);
		path += "post-process.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			post_process.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &post_process.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the post-processing pipeline, layout 
	/// </summary>
	void RayTracer::tearDownPostProcessPipeline()
	{
		destroyPipelineLayout(post_process.layout);
		destroyPipeline(post_process.pipeline);
	}
}
