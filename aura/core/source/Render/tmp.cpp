// ========================================================================== //
// File : tmp.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
// This is all the previous code.

// ------------------------------------------------------------------ //
// ------------------------------------------------------------------ //

/// RayGen launcher and result rays descriptor set layout.
/// </summary>
[[nodiscard]] vk::DescriptorSetLayout Framework::setLauncherRays() const
{
	std::array<vk::DescriptorSetLayoutBinding, 1> const bindings = {
		// - Binding number, descriptor type and count.
		// - Shader stage and sampler.
		vk::DescriptorSetLayoutBinding(
			0U, vk::DescriptorType::eUniformBuffer, 1U,
			vk::ShaderStageFlagBits::eCompute, nullptr
		)
	};
	return Utils::createDescriptorSetLayout(nucleus, {}, bindings);
}
/// <summary>
/// Global storage image descriptor set layout.
/// </summary>
[[nodiscard]] vk::DescriptorSetLayout Framework::setImage() const
{
	std::array<vk::DescriptorSetLayoutBinding, 1> const bindings = {
		// - Binding number, descriptor type and count.
		// - Shader stage and sampler.
		vk::DescriptorSetLayoutBinding(
			0U, vk::DescriptorType::eStorageImage, 1U,
			vk::ShaderStageFlagBits::eCompute, nullptr
		)
	};
	return Utils::createDescriptorSetLayout(nucleus, {}, bindings);
}
/// <summary>
/// Pipeline layout for the RayGen shader.
/// </summary>
[[nodiscard]] vk::PipelineLayout Framework::layoutRayGenTest() const
{
	std::array<vk::DescriptorSetLayout, 2> descriptors {
		set_image, set_launcher_rays
	};
	std::array<vk::PushConstantRange, 1> pushes {
		// - Push constant offset and size.
		vk::PushConstantRange(
			vk::ShaderStageFlagBits::eCompute,
			0U, sizeof(RandomPush)
		)
	};
	return Utils::createPipelineLayout(nucleus, {}, descriptors, pushes);
}
/// <summary>
/// RayGen shader module.
/// </summary>
[[nodiscard]] vk::ShaderModule Framework::moduleRayGenTest() const
{
	auto path = std::string(shader_folder);
	path += "ray-gen_test.spv";
	return Utils::createShaderModule(nucleus, {}, path.c_str());
}
// ------------------------------------------------------------------ //
// Buffer Launcher.
// ------------------------------------------------------------------ //
/// <summary>
/// Ray Launcher uniform buffer.
/// </summary>
[[nodiscard]] vk::Buffer Resources::bufferLauncher() const
{
	std::array<std::uint32_t, 1> families = { nucleus.compute.family };
	return Utils::createBuffer(nucleus,
		{}, sizeof(RayLauncher), vk::BufferUsageFlagBits::eUniformBuffer,
		families);
}
/// <summary>
/// Allocate ray launcher memory.
/// </summary>
[[nodiscard]] vk::DeviceMemory Resources::memoryLauncher() const
{
	auto const requirements = nucleus.device.getBufferMemoryRequirements(
		buffer_launcher, nucleus.dynamic_dispatch);
	auto const optimal = vk::MemoryPropertyFlags(
		vk::MemoryPropertyFlagBits::eDeviceLocal
		| vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent);
	auto const required = vk::MemoryPropertyFlags(
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent);
	vk::DeviceMemory memory(Utils::createMemory(nucleus,
		requirements, optimal, required));
	// Bind ray launcher memory.
	nucleus.device.bindBufferMemory(
		buffer_launcher, memory,
		0U, nucleus.dynamic_dispatch);
	return memory;
}
/// <summary>
/// Returns the ray launcher info descriptor.
/// </summary>
vk::DescriptorBufferInfo Resources::infoLauncher() const
{
	return vk::DescriptorBufferInfo(
		buffer_launcher, 0U, sizeof(RayLauncher));
}

// ------------------------------------------------------------------ //
// Descriptors.
// ------------------------------------------------------------------ //
/// <summary>
/// Creates the descriptor pool for all descriptors.
/// </summary>
[[nodiscard]] vk::DescriptorPool Resources::descriptorPool() const
{
	vk::DescriptorPoolCreateFlags flags {};
	std::array<vk::DescriptorPoolSize, 3> sizes {
		// Size type and count
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 3U),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1U),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1U)
	};
	return Utils::createDescriptorPool(nucleus, flags, sizes, 4U);
}
/// <summary>
/// Allocates and writes launcher and rays set.
/// </summary>
vk::DescriptorSet Resources::dSetLauncherRays() const
{
	std::array<vk::DescriptorSetLayout, 1U> const layouts {
		framework.set_launcher_rays };
	auto const result = Utils::allocDescriptorSets(nucleus, descriptor_pool, layouts);
	vk::DescriptorSet set { result[0] };

	std::array<vk::DescriptorBufferInfo, 2> infos {
		infoLauncher(), infoRays() };
	std::array<vk::WriteDescriptorSet, 2> writes {
		// - Destination set, binding and array element, count.
		// - Type and info(Image, Buffer, Texel).
		vk::WriteDescriptorSet(set, 0U, 0U, 1U,
		vk::DescriptorType::eUniformBuffer, nullptr, &infos[0], nullptr),
		vk::WriteDescriptorSet(set, 1U, 0U, 1U,
		vk::DescriptorType::eStorageBuffer, nullptr, &infos[1], nullptr)
	};
	nucleus.device.updateDescriptorSets(writes, {}, nucleus.dynamic_dispatch);
	return set;
}
/// <summary>
/// Allocates and writes a set for each image.
/// </summary>
std::vector<vk::DescriptorSet> Resources::dSetsImage() const
{
	auto const n_images = static_cast<std::uint32_t>(chain_images.size());
	auto sets = std::vector<vk::DescriptorSet>(n_images);
	for(std::uint32_t i = 0; i < n_images; ++i)
	{
		std::array<vk::DescriptorSetLayout, 1U> const layouts {
			framework.set_image };
		auto const result = Utils::allocDescriptorSets(nucleus, descriptor_pool, layouts);
		sets[i] = result[0];

		auto const infos = infoChainImage(i);
		std::array<vk::WriteDescriptorSet, 1> writes {
			// - Destination set, binding and array element, count.
			// - Type and info(Image, Buffer, Texel).
			vk::WriteDescriptorSet(sets[i], 0U, 0U, 1U,
			vk::DescriptorType::eStorageImage, &infos, nullptr, nullptr)
		};
		nucleus.device.updateDescriptorSets(writes, {}, nucleus.dynamic_dispatch);
	}
	return sets;
}
}

// ------------------------------------------------------------------ //
// ------------------------------------------------------------------ //

/// <summary>
/// RayGen work group sizes.
/// </summary>
static constexpr std::uint32_t raygen_group_size[3] = { 8U, 8U, 1U };
static constexpr auto shader_folder = "../aura/core/shaders/";

// Prepare commands and sync.
acquisition_fence = render->createFence({});
transition_semaphore = render->createSemaphore({});

render->waitIdle();
render->destroySemaphore(transition_semaphore);
render->destroyFence(acquisition_fence);
render->freeCommandBuffer(compute_pool,
	static_cast<std::uint32_t>(secondary_transitions.size()), secondary_transitions.data());
render->freeCommandBuffer(compute_pool,
	1U, &compute_primary);
render->destroyCommandPool(compute_pool);


render->beginRecord({}, {}, secondary_transitions[0]);
render->recordChainImageLayoutTransition(frame_index,
	vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
	{}, {},
	vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
	secondary_transitions[0]);
render->endRecord(secondary_transitions[0]);

render->beginRecord({}, {}, secondary_transitions[1]);
render->recordChainImageLayoutTransition(frame_index,
	vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
	{}, {},
	vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
	secondary_transitions[1]);
render->endRecord(secondary_transitions[1]);

render->beginRecord({}, {}, compute_primary);
render->executeCommands(compute_primary,
	static_cast<std::uint32_t>(secondary_transitions.size()), secondary_transitions.data());
render->endRecord(compute_primary);

vk::PipelineStageFlags const stage(vk::PipelineStageFlagBits::eBottomOfPipe);
std::array<vk::SubmitInfo, 1U> submits {
	// Semaphores count and info and stage flags.
	// Command buffer count and info, and signal semaphores count and info.
	vk::SubmitInfo(
		0U, nullptr, &stage,
		1U, &compute_primary, 1U, &transition_semaphore)
};
render->submitComputeCommands(
	static_cast<std::uint32_t>(submits.size()), submits.data(), vk::Fence());


// ------------------------------------------------------------------ //
// Structure builders.
// ------------------------------------------------------------------ //
/// <summary>
/// Builds a ray launcher according to the given camera settings.
/// </summary>
static Aura::Core::Render::RayLauncher buildRayLauncher(
	Aura::Core::Settings const & settings,
	Aura::Core::Environment::Camera const & camera,
	Aura::Core::Environment::Transform const & transform);
// ------------------------------------------------------------------ //
// Structure builders.
// ------------------------------------------------------------------ //
/// <summary>
/// Builds a ray launcher according to the given camera settings.
/// </summary>
[[nodiscard]] Aura::Core::Render::RayLauncher Render::buildRayLauncher(
	Aura::Core::Settings const & settings,
	Aura::Core::Environment::Camera const & camera,
	Aura::Core::Environment::Transform const & transform)
{
	// Pre-known values.
	constexpr gpu_real pi = static_cast<gpu_real>(3.141592653589793);
	constexpr gpu_real to_radians = pi / static_cast<gpu_real>(180.0);
	constexpr gpu_real two = static_cast<gpu_real>(2.0);
	// Update camera points and vectors.
	gpu_vec3 const lookfrom = gpu_vec3(
		transform.translation * transform.rotation * transform.scaling
		* gpu_vec4(camera.look_from, 1.0)
	);
	gpu_vec3 const lookat = gpu_vec3(
		transform.translation * transform.rotation * transform.scaling
		* gpu_vec4(camera.look_at, 1.0)
	);
	gpu_vec3 const vup = gpu_vec3(
		transform.translation * transform.rotation * transform.scaling
		* gpu_vec4(camera.v_up, 1.0)
	);
	// Calculate helper values.
	gpu_real const aspect =
		static_cast<gpu_real>(settings.ui.width) /
		static_cast<gpu_real>(settings.ui.height);
	gpu_real const theta = camera.v_fov * to_radians;
	gpu_real const half_height = tan(theta / two);
	gpu_real const half_width = aspect * half_height;
	// Calculate ray launcher specification.
	auto ray_launcher = RayLauncher();
	ray_launcher.lens_radius = camera.aperture / two;
	ray_launcher.origin = lookfrom;
	ray_launcher.w = glm::normalize(lookfrom - lookat);
	ray_launcher.u = glm::normalize(glm::cross(vup, ray_launcher.w));
	ray_launcher.v = -glm::cross(ray_launcher.w, ray_launcher.u);
	ray_launcher.vertical = half_height * camera.focus * ray_launcher.v;
	ray_launcher.horizontal = half_width * camera.focus * ray_launcher.u;
	ray_launcher.corner = ray_launcher.origin
		+ ray_launcher.vertical + ray_launcher.horizontal
		- camera.focus * ray_launcher.w;
	ray_launcher.vertical += ray_launcher.vertical;
	ray_launcher.horizontal += ray_launcher.horizontal;
	return ray_launcher;
}

// ------------------------------------------------------------------ //
// Command recording.
// ------------------------------------------------------------------ //
/// <summary>
/// Records a test RayGen dispatch.
/// </summary>
void Render::recordTestRayGen(std::uint32_t const & frame_index,
	vk::CommandBuffer const & command) const
{
	command.bindPipeline(vk::PipelineBindPoint::eCompute,
		framework->pipeline_raygen_test, nucleus->dynamic_dispatch);
	std::array<vk::DescriptorSet, 2> sets {
		resources->dsets_image[frame_index], resources->dset_launcher_rays };
	command.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
		framework->layout_raygen_test, 0U,
		static_cast<std::uint32_t>(sets.size()), sets.data(),
		0U, nullptr, nucleus->dynamic_dispatch);
	command.dispatch(
		nucleus->settings.ui.width / nucleus->settings.render.raygen_group_size[0],
		nucleus->settings.ui.height / nucleus->settings.render.raygen_group_size[1],
		1,
		nucleus->dynamic_dispatch);
}
/// <summary>
/// Records a RayGen dispatch.
/// </summary>
void Render::recordRayGen(vk::CommandBuffer const & command) const
{
	command.bindPipeline(vk::PipelineBindPoint::eCompute,
		framework->pipeline_raygen_test, nucleus->dynamic_dispatch);
	std::array<vk::DescriptorSet, 1> sets {
		resources->dset_launcher_rays };
	command.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
		framework->layout_raygen_test, 0U,
		static_cast<std::uint32_t>(sets.size()), sets.data(),
		0U, nullptr, nucleus->dynamic_dispatch);
	command.dispatch(
		nucleus->settings.ui.width / nucleus->settings.render.raygen_group_size[0],
		nucleus->settings.ui.height / nucleus->settings.render.raygen_group_size[1],
		1,
		nucleus->dynamic_dispatch);
}
