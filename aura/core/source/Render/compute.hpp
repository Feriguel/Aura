// ========================================================================== //
// File : compute.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER_COMPUTE
#define AURACORE_RENDER_COMPUTE
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include "framework.hpp"
// Standard includes.
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
	/// Structure containing all the required information to create a pipeline.
	/// </summary>
	struct ComputeInfo
	{
		vk::PipelineShaderStageCreateFlags shader_flags;
		vk::ShaderModule shader;
		std::string entry;
		vk::PipelineLayout layout;
		vk::PipelineCreateFlags flags;
	};
	class VulkanCompute : public VulkanFramework
	{
		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		protected:
		/// <summary>
		/// Sets-up the base core and starts rendering.
		/// </summary>
		explicit VulkanCompute(
			vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
			vk::PhysicalDevice const & physical_device, vk::Device const & device) :
			VulkanFramework(dispatch, instance, physical_device, device)
		{}

		// ------------------------------------------------------------------ //
		// Compute pipelines.
		// ------------------------------------------------------------------ //
		/// <summary>
		/// Creates N pipelines as stated in the supplied compute infos.
		/// Doesn't support specialization nor pipeline derivates.
		/// Throws any error that might occur.
		/// </summary>
		void createComputePipelines(vk::PipelineCache const & cache,
			std::uint32_t const & n_pipelines, ComputeInfo const * const p_infos,
			vk::Pipeline * const p_pipelines) const
		{
			std::vector<vk::PipelineShaderStageCreateInfo> shader_stages { n_pipelines };
			std::vector<vk::ComputePipelineCreateInfo> create_infos { n_pipelines };
			for(std::uint32_t i { 0U }; i < n_pipelines; ++i)
			{
				shader_stages[i]
					.setFlags(p_infos[i].shader_flags)
					.setStage(vk::ShaderStageFlagBits::eCompute)
					.setModule(p_infos[i].shader)
					.setPName(p_infos[i].entry.c_str());
				create_infos[i]
					.setFlags(p_infos[i].flags)
					.setStage(shader_stages[i])
					.setLayout(p_infos[i].layout);
			}
			vk::Result const result { device.createComputePipelines(
				cache, n_pipelines, create_infos.data(), nullptr, p_pipelines, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "createComputePipelines"); }
		}
		/// <summary>
		/// Destroys N pipelines.
		/// </summary>
		void destroyPipelines(std::uint32_t const & n_pipelines,
			vk::Pipeline * const p_pipelines) const noexcept
		{
			for(std::uint32_t i { 0U }; i < n_pipelines; ++i)
			{
				device.destroyPipeline(p_pipelines[i], nullptr, dispatch);
			}
		}
	};
}

#endif
