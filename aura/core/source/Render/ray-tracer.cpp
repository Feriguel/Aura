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
		std::uint32_t const & width, std::uint32_t const & height,
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
	/// Records a ray gen operation.
	/// </summary>
	void RayTracer::recordRayGen(RandomOffset & push, vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, gen.pipeline, dispatch);
		std::array<vk::DescriptorSet, 3U> sets { ray_launcher.set, rays_and_hits.set, pixels_state.set };
		command.bindDescriptorSets(bind_point, gen.layout, 0U,
			3U, sets.data(), 0U, nullptr, dispatch);
		command.pushConstants(gen.layout, vk::ShaderStageFlagBits::eCompute,
			0U, sizeof(RandomOffset), reinterpret_cast<void *>(&push), dispatch);
		command.dispatch(width / group_size[0U], height / group_size[1U],
			group_size[2U], dispatch);
	}
	/// <summary>
	/// Records a intersect operation.
	/// </summary>
	void RayTracer::recordIntersect(vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, intersect.pipeline, dispatch);
		std::array<vk::DescriptorSet, 1U> sets { rays_and_hits.set };
		command.bindDescriptorSets(bind_point, intersect.layout, 0U,
			1U, sets.data(), 0U, nullptr, dispatch);
		command.dispatch(width / group_size[0U], height / group_size[1U],
			group_size[2U], dispatch);
	}
	/// <summary>
	/// Records a absorption and colouring operation.
	/// </summary>
	void RayTracer::recordColour(vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, colour.pipeline, dispatch);
		std::array<vk::DescriptorSet, 2U> sets { rays_and_hits.set, pixels_state.set };
		command.bindDescriptorSets(bind_point, colour.layout, 0U,
			2U, sets.data(), 0U, nullptr, dispatch);
		command.dispatch(width / group_size[0U], height / group_size[1U],
			group_size[2U], dispatch);
	}
	/// <summary>
	/// Records a scatter operation.
	/// </summary>
	void RayTracer::recordScatter(vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, scatter.pipeline, dispatch);
		std::array<vk::DescriptorSet, 1U> sets { rays_and_hits.set };
		command.bindDescriptorSets(bind_point, scatter.layout, 0U,
			1U, sets.data(), 0U, nullptr, dispatch);
		command.dispatch(width / group_size[0U], height / group_size[1U],
			group_size[2U], dispatch);
	}
	/// <summary>
	/// Records a post-processing operation.
	/// </summary>
	void RayTracer::recordPostProcess(vk::CommandBuffer const & command) const
	{
		vk::PipelineBindPoint bind_point { vk::PipelineBindPoint::eCompute };
		command.bindPipeline(bind_point, post_process.pipeline, dispatch);
		std::array<vk::DescriptorSet, 2U> sets { pixels_state.set, chain_image.set };
		command.bindDescriptorSets(bind_point, post_process.layout, 0U,
			2U, sets.data(), 0U, nullptr, dispatch);
		command.dispatch(width / group_size[0U], height / group_size[1U],
			group_size[2U], dispatch);
	}

	// ------------------------------------------------------------------ //
	// Resource updates.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Updates the ray launcher using the camera in the scene.
	/// </summary>
	void RayTracer::updateRayLauncher(std::uint32_t const ray_depth)
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
			launcher.u = -glm::normalize(glm::cross(vup, launcher.w));
			launcher.v = glm::cross(launcher.w, launcher.u);
			launcher.vertical = half_height * camera.focus * -launcher.v;
			launcher.horizontal = half_width * camera.focus * -launcher.u;
			launcher.corner = launcher.origin + launcher.vertical + launcher.horizontal - camera.focus * launcher.w;
			launcher.vertical *= 2.0f;
			launcher.horizontal *= -2.0f;
			launcher.max_bounces = ray_depth;
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

		std::array<std::future<void>, 3> const jobs {
			thread_pool.enqueue([&] { setUpRayLauncher(); }),
			thread_pool.enqueue([&] { setUpRaysAndHits(); }),
			thread_pool.enqueue([&] { setUpPixelsState(); })
		};

		for(std::size_t i { 0U }; i < jobs.size(); ++i)
		{
			if(!jobs[i].valid()) { throw std::future_error(std::future_errc::no_state); }
			jobs[i].wait();
		}
		allocateAllDescriptorSets();

		updateRayLauncherSet();
		updateRaysAndHitsSet();
		updatePixelsStateSet();
	}
	/// <summary>
	/// Bulk tears-down all set-up resources, as well as, the descriptor pool.
	/// </summary>
	void RayTracer::tearDownResources()
	{
		tearDownPixelsState();
		tearDownRaysAndHits();
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
		layouts.emplace_back(rays_and_hits.set_layout);
		layouts.emplace_back(pixels_state.set_layout);

		std::size_t n_sets { layouts.size() };
		sets.resize(n_sets);
		allocateDescriptorSets(pool, static_cast<std::uint32_t>(n_sets), layouts.data(), sets.data());

		chain_image.set = sets[0U];
		ray_launcher.set = sets[1U];
		rays_and_hits.set = sets[2U];
		pixels_state.set = sets[3U];
	}
	/// <summary>
	/// Sets up the descriptor pool for all resources.
	/// </summary>
	void RayTracer::setUpDescriptorPool()
	{
		constexpr std::uint32_t n_sets { 4U };
		constexpr std::uint32_t n_sizes { 3U };
		std::array<vk::DescriptorPoolSize, n_sizes> sizes {
			// - Descriptor type and count.
			vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, 1U },
			vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, 3U },
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
		createBuffer({}, launcher_size, vk::BufferUsageFlagBits::eUniformBuffer,
			1U, &compute_family, buffer);
		vk::MemoryRequirements mem_launcher {};
		device.getBufferMemoryRequirements(buffer, &mem_launcher, dispatch);

		ray_launcher.buffers.resize(n_buffers);
		ray_launcher.buffers[0U].buffer = buffer;
		ray_launcher.buffers[0U].range = launcher_size;
		ray_launcher.buffers[0U].offset = 0U;

		ray_launcher.memories.resize(1U);
		allocateMemory(mem_launcher,
			vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			ray_launcher.memories[0U]);

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
	void RayTracer::setUpRaysAndHits()
	{
		constexpr std::uint32_t n_buffers { 2U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr },
			vk::DescriptorSetLayoutBinding { 1U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), rays_and_hits.set_layout);

		vk::DeviceSize const rays_size { static_cast<vk::DeviceSize>(sizeof(Ray) * width * height) };
		vk::DeviceSize const hits_size { static_cast<vk::DeviceSize>(sizeof(Hit) * width * height) };
		vk::Buffer buffer {};
		createBuffer({}, rays_size + hits_size, vk::BufferUsageFlagBits::eStorageBuffer,
			1U, &compute_family, buffer);
		vk::MemoryRequirements mem_rays_and_hits {};
		device.getBufferMemoryRequirements(buffer, &mem_rays_and_hits, dispatch);

		rays_and_hits.buffers.resize(n_buffers);
		rays_and_hits.buffers[0U].buffer = buffer;
		rays_and_hits.buffers[0U].range = rays_size;
		rays_and_hits.buffers[0U].offset = 0U;
		rays_and_hits.buffers[1U].buffer = buffer;
		rays_and_hits.buffers[1U].range = hits_size;
		rays_and_hits.buffers[1U].offset = rays_size;

		rays_and_hits.memories.resize(1U);
		allocateMemory(mem_rays_and_hits,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			rays_and_hits.memories[0U]);

		device.bindBufferMemory(rays_and_hits.buffers[0U].buffer, rays_and_hits.memories[0U],
			rays_and_hits.buffers[0U].offset, dispatch);
	}
	/// <summary>
	/// Update rays and hits set. This isn't mutable, and should only be
	/// used once.
	/// </summary>
	void RayTracer::updateRaysAndHitsSet()
	{
		std::array<vk::WriteDescriptorSet, 2U> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{rays_and_hits.set, 0U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &rays_and_hits.buffers[0], nullptr},
			vk::WriteDescriptorSet{rays_and_hits.set, 1U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &rays_and_hits.buffers[1], nullptr}
		};
		device.updateDescriptorSets(2U, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys the rays and hits layout, buffers and memory.
	/// </summary>
	void RayTracer::tearDownRaysAndHits()
	{
		destroyBuffer(rays_and_hits.buffers[0U].buffer);
		freeMemory(rays_and_hits.memories[0U]);
		destroyDescriptorSetLayout(rays_and_hits.set_layout);
	}
	/// <summary>
	/// Prepares pixels state layout, buffer and memory.
	/// </summary>
	void RayTracer::setUpPixelsState()
	{
		constexpr std::uint32_t n_buffers { 1U };

		std::array<vk::DescriptorSetLayoutBinding, n_buffers> const binds {
			// - Binding number, descriptor type and count.
			// - Shader stage and sampler.
			vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageBuffer, 1U,
				vk::ShaderStageFlagBits::eCompute, nullptr } };
		createDescriptorSetLayout({}, n_buffers, binds.data(), pixels_state.set_layout);

		vk::DeviceSize const pixels_size { static_cast<vk::DeviceSize>(sizeof(RayLauncher) * width * height) };
		vk::Buffer buffer {};
		createBuffer({}, pixels_size, vk::BufferUsageFlagBits::eStorageBuffer,
			1U, &compute_family, buffer);
		vk::MemoryRequirements mem_pixels {};
		device.getBufferMemoryRequirements(buffer, &mem_pixels, dispatch);

		pixels_state.buffers.resize(n_buffers);
		pixels_state.buffers[0U].buffer = buffer;
		pixels_state.buffers[0U].range = pixels_size;
		pixels_state.buffers[0U].offset = 0U;

		pixels_state.memories.resize(1U);
		allocateMemory(mem_pixels,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			pixels_state.memories[0U]);

		device.bindBufferMemory(pixels_state.buffers[0U].buffer, pixels_state.memories[0U],
			pixels_state.buffers[0U].offset, dispatch);
	}
	/// <summary>
	/// Update pixels state set. This isn't mutable, and should only be
	/// used once.
	/// </summary>
	void RayTracer::updatePixelsStateSet()
	{
		std::array<vk::WriteDescriptorSet, 1U> const writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet{pixels_state.set, 0U, 0U, 1U,
				vk::DescriptorType::eStorageBuffer, nullptr, &pixels_state.buffers[0], nullptr}
		};
		device.updateDescriptorSets(1U, writes.data(), 0U, nullptr, dispatch);
	}
	/// <summary>
	/// Destroys the pixels state layout, buffer and memory.
	/// </summary>
	void RayTracer::tearDownPixelsState()
	{
		destroyBuffer(pixels_state.buffers[0U].buffer);
		freeMemory(pixels_state.memories[0U]);
		destroyDescriptorSetLayout(pixels_state.set_layout);
	}

	// ------------------------------------------------------------------ //
	// Pipelines.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Bulk sets-up all pipelines.
	/// </summary>
	void RayTracer::setUpPipelines(ThreadPool & thread_pool)
	{
		std::array<std::future<void>, 5> const jobs {
			thread_pool.enqueue([&] { setUpGenPipeline(); }),
			thread_pool.enqueue([&] { setUpIntersectPipeline(); }),
			thread_pool.enqueue([&] { setUpColourPipeline(); }),
			thread_pool.enqueue([&] { setUpScatterPipeline(); }),
			thread_pool.enqueue([&] { setUpPostProcessPipeline(); })
		};

		for(std::size_t i { 0U }; i < jobs.size(); ++i)
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
		tearDownScatterPipeline();
		tearDownColourPipeline();
		tearDownIntersectPipeline();
		tearDownGenPipeline();
	}
	/// <summary>
	/// Prepares the ray generation layout and shader module.
	/// </summary>
	void RayTracer::setUpGenPipeline()
	{
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 3U> const set_layouts {
			ray_launcher.set_layout, rays_and_hits.set_layout, pixels_state.set_layout };
		vk::PushConstantRange const push {
			vk::ShaderStageFlagBits::eCompute, 0U, sizeof(RandomOffset) };
		createPipelineLayout({}, 3U, set_layouts.data(), 1U, &push, gen.layout);

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
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 1U> const set_layouts {
			rays_and_hits.set_layout };
		createPipelineLayout({}, 1U, set_layouts.data(), 0U, nullptr, intersect.layout);

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
	/// Prepares the colour layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpColourPipeline()
	{
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 2U> const set_layouts {
			rays_and_hits.set_layout, pixels_state.set_layout };
		createPipelineLayout({}, 2U, set_layouts.data(), 0U, nullptr, colour.layout);

		auto path = std::string(shader_folder);
		path += "colour.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			colour.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &colour.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the colour pipeline, layout 
	/// </summary>
	void RayTracer::tearDownColourPipeline()
	{
		destroyPipelineLayout(colour.layout);
		destroyPipeline(colour.pipeline);
	}
	/// <summary>
	/// Prepares the scatter layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpScatterPipeline()
	{
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 1U> const set_layouts {
			rays_and_hits.set_layout };
		createPipelineLayout({}, 1U, set_layouts.data(), 0U, nullptr, scatter.layout);

		auto path = std::string(shader_folder);
		path += "scatter.spv";
		createShaderModule({}, path.c_str(), shader);

		vk::PipelineShaderStageCreateInfo const stage { {},
			vk::ShaderStageFlagBits::eCompute, shader, "main", nullptr };
		vk::ComputePipelineCreateInfo const create_info { {}, stage,
			scatter.layout, vk::Pipeline(), 0U };
		createComputePipelines(cache, 1U, &create_info, &scatter.pipeline);
		destroyShaderModule(shader);
	}
	/// <summary>
	/// Destroys the scatter pipeline, layout 
	/// </summary>
	void RayTracer::tearDownScatterPipeline()
	{
		destroyPipelineLayout(scatter.layout);
		destroyPipeline(scatter.pipeline);
	}
	/// <summary>
	/// Prepares the post-processing layout, shader module and pipeline.
	/// </summary>
	void RayTracer::setUpPostProcessPipeline()
	{
		vk::PipelineCache cache {};
		vk::ShaderModule shader {};

		std::array<vk::DescriptorSetLayout, 2U> const set_layouts {
			pixels_state.set_layout, chain_image.set_layout };
		createPipelineLayout({}, 2U, set_layouts.data(), 0U, nullptr, post_process.layout);

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
