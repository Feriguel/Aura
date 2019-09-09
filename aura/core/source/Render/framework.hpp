// ========================================================================== //
// File : framework.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER_FRAMEWORK
#define AURACORE_RENDER_FRAMEWORK
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
// Standard includes.
#include <cstdint>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
// External includes.
#pragma warning(disable : 26495)
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
#pragma warning(default : 26495)

namespace Aura::Core
{
	/// <summary>
	/// Core of render. Holds the created device and the references to the main
	/// components of Vulkan.
	/// </summary>
	class VulkanFramework
	{
		protected:
		// External vulkan dispatcher.
		vk::DispatchLoaderDynamic const & dispatch;
		// External vulkan instance.
		vk::Instance const & instance;
		// External vulkan selected physical device.
		vk::PhysicalDevice const & physical_device;
		// Vulkan logic device.
		vk::Device const & device;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		protected:
		/// <summary>
		/// Sets-up the base core and starts rendering.
		/// </summary>
		explicit VulkanFramework(
			vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
			vk::PhysicalDevice const & physical_device, vk::Device const & device) :
			dispatch(dispatch), instance(instance),
			physical_device(physical_device), device(device)
		{}

		// ------------------------------------------------------------------ //
		// Resources.
		// ------------------------------------------------------------------ //
		protected:
		/// <summary>
		/// Creates a descriptors set layouts with the given N bindings.
		/// </summary>
		void createDescriptorSetLayout(vk::DescriptorSetLayoutCreateFlags const & flags,
			std::uint32_t const & n_bindings, vk::DescriptorSetLayoutBinding const * const p_bindings,
			vk::DescriptorSetLayout & set_layout) const
		{
			vk::DescriptorSetLayoutCreateInfo const create_info { flags,
				n_bindings, p_bindings };
			vk::Result const result { device.createDescriptorSetLayout(
				&create_info, nullptr, &set_layout, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createDescriptorSetLayout"); }
		}
		/// <summary>
		/// Destroys a descriptor set layout.
		/// </summary>
		void destroyDescriptorSetLayout(vk::DescriptorSetLayout & set_layout) const noexcept
		{
			device.destroyDescriptorSetLayout(set_layout, nullptr, dispatch);
		}
		/// <summary>
		/// Creates a descriptors set layouts with the given N bindings.
		/// </summary>
		void createDescriptorPool(vk::DescriptorPoolCreateFlags const & flags,
			std::uint32_t const & max_sets, std::uint32_t const & n_sizes,
			vk::DescriptorPoolSize const * const p_sizes, vk::DescriptorPool & pool) const
		{
			vk::DescriptorPoolCreateInfo const create_info { flags, max_sets,
				n_sizes, p_sizes };
			vk::Result const result { device.createDescriptorPool(
				&create_info, nullptr, &pool, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createDescriptorPool"); }
		}
		/// <summary>
		/// Destroys a descriptor set layout.
		/// </summary>
		void destroyDescriptorPool(vk::DescriptorPool & pool) const noexcept
		{
			device.destroyDescriptorPool(pool, nullptr, dispatch);
		}
		/// <summary>
		/// Allocates N descriptor sets at the target data pointer on the given
		/// descriptor pool.
		/// Throws any error that might occur.
		/// </summary>
		void allocateDescriptorSets(vk::DescriptorPool const & pool,
			std::uint32_t const & n_sets, vk::DescriptorSetLayout const * const p_layouts,
			vk::DescriptorSet * const p_sets) const
		{
			vk::DescriptorSetAllocateInfo const alloc_info { pool, n_sets, p_layouts };
			vk::Result const result { device.allocateDescriptorSets(
				&alloc_info, p_sets, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "allocateDescriptorSets"); }
		}
		/// <summary>
		/// Frees N buffers at the target data pointer on the given command pool.
		/// </summary>
		void freeDescriptorSets(vk::DescriptorPool const & pool,
			std::uint32_t const & n_descriptors, vk::DescriptorSet * const p_descriptors) const noexcept
		{
			device.freeDescriptorSets(pool, n_descriptors, p_descriptors, dispatch);
		}
		/// <summary>
		/// Creates an image view for each swap-chain image.
		/// </summary>
		void createImageView(vk::ImageViewCreateFlags const & flags,
			vk::Image const & image, vk::ImageViewType const & type, vk::Format const & format,
			vk::ComponentMapping const & mapping, vk::ImageSubresourceRange const & subresource,
			vk::ImageView & view)
		{
			vk::ImageViewCreateInfo const create_info { flags, image, type,
				format, mapping, subresource };
			vk::Result const result { device.createImageView(
				&create_info, nullptr, &view, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createImageView"); }
		}
		/// <summary>
		/// Destroys all swap-chain image views.
		/// </summary>
		void destroyImageView(vk::ImageView & view) const noexcept
		{
			device.destroyImageView(view, nullptr, dispatch);
		}
		/// <summary>
		/// Creates a buffer with the given size and usage and accessing queues.
		/// Throws any error that might occur.
		/// </summary>
		void createBuffer(vk::BufferCreateFlags const & flags,
			vk::DeviceSize const & size, vk::BufferUsageFlags const & usage,
			std::uint32_t const & n_families, std::uint32_t const * const p_families,
			vk::Buffer & buffer) const
		{
			if(n_families == 1)
			{
				vk::BufferCreateInfo const create_info { flags, size, usage,
					vk::SharingMode::eExclusive, n_families, p_families };
				vk::Result const result { device.createBuffer(
					&create_info, nullptr, &buffer, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "createBuffer"); }
			}
			else
			{
				vk::BufferCreateInfo const create_info { flags, size, usage,
					vk::SharingMode::eConcurrent, n_families, p_families };
				vk::Result const result { device.createBuffer(
					&create_info, nullptr, &buffer, dispatch) };
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "createBuffer"); }
			}
		}
		/// <summary>
		/// Destroys a buffer.
		/// </summary>
		void destroyBuffer(vk::Buffer & buffer) const noexcept
		{
			device.destroyBuffer(buffer, nullptr, dispatch);
		}
		/// <summary>
		/// Allocates a memory with the given memory requirements and either the
		/// optimal flags, or, if the optimal fail, the minimum required.
		/// Throws any error that might occur.
		/// </summary>
		void allocateMemory(
			vk::MemoryRequirements const & requirements,
			vk::MemoryPropertyFlags const & optimal_properties,
			vk::MemoryPropertyFlags const & required_properties,
			vk::DeviceMemory & memory)
		{
			vk::PhysicalDeviceMemoryProperties properties {};
			physical_device.getMemoryProperties(&properties, dispatch);
			std::uint32_t type_index { 0U };
			if(!findMemoryType(properties, requirements, optimal_properties, type_index))
			{
				if(!findMemoryType(properties, requirements, required_properties, type_index))
				{
					std::string message { "Required memory(" };
					message += vk::to_string(required_properties) + ") not found.";
					throw std::exception(message.c_str());
				}
			}
			vk::MemoryAllocateInfo const allocate_info { requirements.size, type_index };
			vk::Result const result { device.allocateMemory(
				&allocate_info, nullptr, &memory, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "allocateMemory"); }
		}
		/// <summary>
		/// Frees an allocated memory.
		/// </summary>
		void freeMemory(vk::DeviceMemory & memory) const noexcept
		{
			device.freeMemory(memory, nullptr, dispatch);
		}

		// ------------------------------------------------------------------ //
		//Pipeline.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Creates a shader module using the SPIR-V file at path.
		/// Throws any error that might occur.
		/// </summary>
		void createShaderModule(vk::ShaderModuleCreateFlags const & flags,
			std::string const & path, vk::ShaderModule & shader) const
		{
			std::vector<char> binary {};
			readShaderBinary(path, binary);
			vk::ShaderModuleCreateInfo const create_info { flags, binary.size(),
				reinterpret_cast<std::uint32_t const *>(binary.data()) };
			vk::Result const result { device.createShaderModule(
				&create_info, nullptr, &shader, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createShaderModule"); }
		}
		/// <summary>
		/// Destroys a shader module.
		/// </summary>
		void destroyShaderModule(vk::ShaderModule & shader) const noexcept
		{
			device.destroyShaderModule(shader, nullptr, dispatch);
		}
		/// <summary>
		/// Creates a pipeline layout with the given set layout and push
		/// constants array.
		/// Throws any error that might occur.
		/// </summary>
		void createPipelineLayout(vk::PipelineLayoutCreateFlags const & flags,
			std::uint32_t const & n_sets, vk::DescriptorSetLayout const * const p_sets,
			std::uint32_t const & n_push, vk::PushConstantRange const * const p_push,
			vk::PipelineLayout & layout) const
		{
			vk::PipelineLayoutCreateInfo const create_info { flags,
				n_sets, p_sets, n_push, p_push };
			vk::Result const result { device.createPipelineLayout(
				&create_info, nullptr, &layout, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createPipelineLayout"); }
		}
		/// <summary>
		/// Destroys a pipeline layout.
		/// </summary>
		void destroyPipelineLayout(vk::PipelineLayout & layout) const noexcept
		{
			device.destroyPipelineLayout(layout, nullptr, dispatch);
		}
		/// <summary>
		/// Creates N pipelines based on the supplied create infos. Multiple
		/// pipeline creations should be used only if there are derivatives as
		/// multi-threading pipeline creation is better otherwise.
		/// Throws any error that might occur.
		/// </summary>
		void createComputePipelines(vk::PipelineCache const & cache, std::uint32_t const & n_pipelines,
			vk::ComputePipelineCreateInfo const * const create_infos, vk::Pipeline * const p_pipelines) const
		{
			vk::Result const result { device.createComputePipelines(
				cache, n_pipelines, create_infos, nullptr, p_pipelines, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createComputePipelines"); }
		}
		/// <summary>
		/// Destroys a pipeline.
		/// </summary>
		void destroyPipeline(vk::Pipeline & pipeline) const noexcept
		{
			device.destroyPipeline(pipeline, nullptr, dispatch);
		}

		// ------------------------------------------------------------------ //
		// Helpers.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Finds a memory with the given requirements and flags requirements.
		/// </summary>
		bool findMemoryType(
			vk::PhysicalDeviceMemoryProperties const & memory_properties,
			vk::MemoryRequirements const & requirements,
			vk::MemoryPropertyFlags const & required_properties,
			std::uint32_t & type_index) const noexcept
		{
			std::uint32_t const memory_count = memory_properties.memoryTypeCount;
			for(std::uint32_t i = 0; i < memory_count; ++i)
			{
				std::uint32_t const memory_type_bits =
					(static_cast<std::uint32_t>(1) << i);
				bool const is_type =
					requirements.memoryTypeBits & memory_type_bits;
				vk::MemoryPropertyFlags const flags(
					memory_properties.memoryTypes[i].propertyFlags);
				bool const is_fit =
					(flags & required_properties) == required_properties;
				if(is_type && is_fit)
				{
					type_index = i;
					return true;
				}
			}
			return false;
		}
		/// <summary>
		/// Loads the shader at the given path.
		/// </summary>
		void readShaderBinary(std::string const & path, std::vector<char> & binary) const
		{
			std::ifstream file(path, std::ios::ate | std::ios::binary);
			if(!file.is_open())
			{
				auto message = std::string("Failed to open file at: ");
				message += path;
				throw std::exception(message.c_str());
			}
			auto const n_chars = static_cast<std::size_t>(file.tellg());
			binary.resize(n_chars);
			file.seekg(0, file.beg).read(binary.data(), n_chars);
			file.close();
		}
	};
}

#endif
