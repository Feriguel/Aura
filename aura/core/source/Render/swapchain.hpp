// ========================================================================== //
// File : swapchain.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RENDER_SWAPCHAIN
#define AURACORE_RENDER_SWAPCHAIN
// Internal includes.
#include "framework.hpp"
// Standard includes.
#include <cstdint>
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
	/// Structure containing all the layout and set of a chain image.
	/// </summary>
	struct ChainResource
	{
		// Resource descriptor set layout.
		vk::DescriptorSetLayout set_layout {};
		// Resource descriptor set handles.
		vk::DescriptorSet set {};
	};
	/// <summary>
	/// Swap-chain initialisation defaults and storage of the actual created swap-chain state.
	/// </summary>
	struct SwapChainInfo
	{
		// Presentation mode of chain images.
		vk::PresentModeKHR present_mode { vk::PresentModeKHR::eMailbox };
		// Format and colour space of swap-chain images.
		vk::SurfaceFormatKHR surface_format { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		// Default usage flags of swap-chain images.
		vk::ImageUsageFlags usage_flags { vk::ImageUsageFlagBits::eStorage };
		// Composite alpha.
		vk::CompositeAlphaFlagBitsKHR alpha_mode { vk::CompositeAlphaFlagBitsKHR::eOpaque };
		// Image component mapping order.
		vk::ComponentMapping mapping {};
	};
	/// <summary>
	/// Extension to the framework which creates an swap-chain and all requirements for its use.
	/// Comes with a single pre-initialised descriptor, if not explicitly disabled. These ain't allocated.
	/// </summary>
	class VulkanSwapchain : public VulkanFramework
	{
		protected:
		// Vulkan framework handle.
		vk::SurfaceKHR const & surface;
		// Vulkan swap-chain image extent.
		vk::Extent2D extent;
		// Current swap-chain options.
		SwapChainInfo info;
		// Vulkan swap-chain handle.
		vk::SwapchainKHR chain;
		// Vulkan swap-chain image views.
		std::vector<vk::Image> chain_images;
		// Vulkan swap-chain image views.
		std::vector<vk::ImageView> chain_views;
		// Chain images.
		ChainResource chain_image;

		// ------------------------------------------------------------------ //
		// Set-up and tear-down.
		// ------------------------------------------------------------------ //
		protected:
		/// <summary>
		/// Sets-up both swap-chain and swap-chain image views.
		/// </summary>
		explicit VulkanSwapchain(
			vk::DispatchLoaderDynamic const & dispatch, vk::Instance const & instance,
			vk::PhysicalDevice const & physical_device, vk::Device const & device,
			vk::SurfaceKHR const & surface, vk::Extent2D const & base_extent,
			std::vector<std::uint32_t> accessing_families, bool const descriptor = true) :
			VulkanFramework(dispatch, instance, physical_device, device),
			surface(surface), extent(base_extent)
		{
			std::sort(accessing_families.begin(), accessing_families.end());
			auto last = std::unique(accessing_families.begin(), accessing_families.end());
			accessing_families.erase(last, accessing_families.end());

			createSwapChain(accessing_families);
			createChainImageViews();
			if(descriptor)
			{
				setUpChainImage();
			}
		}
		/// <summary>
		/// Tears-down both swap-chain and swap-chain image views.
		/// </summary>
		virtual ~VulkanSwapchain() noexcept
		{
			tearDownChainImage();
			destroyChainImageViews();
			destroySwapChain();
		}

		// ------------------------------------------------------------------ //
		// Image acquisition and presentation.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Retrieves a new frame for render. Doesn't wait if timeout is 0.
		/// Must only be used by the main thread.
		/// Throws any error that might occur.
		/// </summary>
		bool acquireframe(vk::Semaphore const & semaphore, vk::Fence const & fence,
			std::uint64_t const & timeout, std::uint32_t & frame_index) const
		{
			vk::Result const result { device.acquireNextImageKHR(
				chain, timeout, semaphore, fence, &frame_index, dispatch) };
			if(result == vk::Result::eTimeout || result == vk::Result::eNotReady)
			{ return false; }
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "acquireNextImageKHR"); }
			return true;
		}
		/// <summary>
		/// Display frame with the given index, returns false if index out of limits.
		/// Must only be used by the main thread.
		/// Throws any error that might occur.
		/// </summary>
		void displayFrame(std::uint32_t const & n_semaphores, vk::Semaphore const * const p_semaphores,
			std::uint32_t const & frame_index, vk::Queue const & queue) const
		{
			if(frame_index >= static_cast<std::uint32_t>(chain_views.size()))
			{ throw std::exception("Frame index out of bounds."); }
			vk::PresentInfoKHR const present_info { n_semaphores, p_semaphores,
				1U, &chain, &frame_index, nullptr };
			vk::Result const result { queue.presentKHR(
				&present_info, dispatch) };
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "presentKHR"); }
		}
		/// <summary>
		/// Records a pipeline barrier with a chain image layout transition.
		/// </summary>
		void recordChainImageLayoutTransition(std::uint32_t const & frame_index,
			vk::AccessFlags const & src_access_flags, vk::AccessFlags const & dst_access_flags,
			vk::ImageLayout const & src_layout, vk::ImageLayout const & dst_layout,
			std::uint32_t const & src_family, std::uint32_t const & dst_family,
			vk::PipelineStageFlags const & src_stage, vk::PipelineStageFlags const & dst_stage,
			vk::CommandBuffer const & command) const
		{
			vk::ImageSubresourceRange const subresource {
				vk::ImageAspectFlagBits::eColor, 0U, 1U, 0U, 1U };
			vk::ImageMemoryBarrier barrier(
				src_access_flags, dst_access_flags,
				src_layout, dst_layout,
				src_family, dst_family,
				chain_images[frame_index], subresource);
			command.pipelineBarrier(
				src_stage, dst_stage,
				vk::DependencyFlags(),
				0U, nullptr, 0U, nullptr, 1U, &barrier,
				dispatch);
		}

		// ------------------------------------------------------------------ //
		// Resources.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Prepares chain image layout.
		/// </summary>
		void setUpChainImage()
		{
			std::array<vk::DescriptorSetLayoutBinding, 1U> const binds {
				// - Binding number, descriptor type and count.
				// - Shader stage and sampler.
				vk::DescriptorSetLayoutBinding { 0U, vk::DescriptorType::eStorageImage, 1U,
					vk::ShaderStageFlagBits::eCompute, nullptr } };
			createDescriptorSetLayout({}, 1U, binds.data(), chain_image.set_layout);
		}
		public:
		/// <summary>
		/// Updates chain image according to the frame index.
		/// </summary>
		void updateChainImageSet(std::uint32_t frame_index)
		{
			std::array<vk::DescriptorImageInfo, 1U> const images {
				// - Sampler, view, layout.
				vk::DescriptorImageInfo {nullptr, chain_views[frame_index], vk::ImageLayout::eGeneral}
			};
			std::array<vk::WriteDescriptorSet, 1U> const writes {
				// - Destination set, binding and array element, count.
				// - Type and info(Image, Buffer, Texel).
				vk::WriteDescriptorSet{chain_image.set, 0U, 0U, 1U,
					vk::DescriptorType::eStorageImage, &images[0U], nullptr, nullptr}
			};
			device.updateDescriptorSets(1U, writes.data(), 0U, nullptr, dispatch);
		}
		private:
		/// <summary>
		/// Destroys chain image layout.
		/// </summary>
		void tearDownChainImage()
		{
			destroyDescriptorSetLayout(chain_image.set_layout);
		}

		// ------------------------------------------------------------------ //
		// Swap-chain and images.
		// ------------------------------------------------------------------ //
		private:
		void createSwapChain(std::vector<std::uint32_t> const & accessing_families)
		{
			vk::Result result { vk::Result::eSuccess };

			vk::SurfaceCapabilitiesKHR surface_capabilities {};
			result = physical_device.getSurfaceCapabilitiesKHR(
				surface, &surface_capabilities, dispatch);
			if(result != vk::Result::eSuccess)
			{ vk::throwResultException(result, "getSurfaceCapabilitiesKHR"); }

			ensurePresentMode();
			ensureSurfaceFormat();
			ensureExtent(surface_capabilities);

			std::uint32_t n_images { 0U };
			selectMinimumChainImages(surface_capabilities, n_images);

			if(accessing_families.size() == 1U)
			{
				vk::SwapchainCreateInfoKHR const create_info { {}, surface,
					n_images, info.surface_format.format, info.surface_format.colorSpace,
					extent, 1U, info.usage_flags, vk::SharingMode::eExclusive,
					static_cast<std::uint32_t>(accessing_families.size()),
					accessing_families.data(), surface_capabilities.currentTransform,
					info.alpha_mode, info.present_mode, static_cast<vk::Bool32>(false) };
				result = device.createSwapchainKHR(
					&create_info, nullptr, &chain, dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "createSwapchainKHR"); }
			}
			else
			{
				vk::SwapchainCreateInfoKHR const create_info { {}, surface,
					n_images, info.surface_format.format, info.surface_format.colorSpace,
					extent, 1U, info.usage_flags, vk::SharingMode::eExclusive,
					static_cast<std::uint32_t>(accessing_families.size()),
					accessing_families.data(), surface_capabilities.currentTransform,
					info.alpha_mode, info.present_mode, static_cast<vk::Bool32>(false) };
				result = device.createSwapchainKHR(
					&create_info, nullptr, &chain, dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "createSwapchainKHR"); }
			}
			device.getSwapchainImagesKHR(
				chain, &n_images, nullptr, dispatch);
			chain_images.resize(n_images);
			device.getSwapchainImagesKHR(
				chain, &n_images, chain_images.data(), dispatch);
		}
		void destroySwapChain() noexcept
		{
			device.destroySwapchainKHR(
				chain, nullptr, dispatch);
		}
		/// <summary>
		/// Creates an image view for each swap-chain image.
		/// </summary>
		void createChainImageViews()
		{
			std::uint32_t const n_images { static_cast<std::uint32_t>(chain_images.size()) };
			chain_views.resize(n_images);
			for(std::uint32_t i { 0U }; i < n_images; ++i)
			{
				vk::ImageSubresourceRange const subresource {
					vk::ImageAspectFlagBits::eColor, 0U, 1U, 0U, 1U };
				createImageView({}, chain_images[i], vk::ImageViewType::e2D,
					info.surface_format.format, info.mapping, subresource, chain_views[i]);
			}
		}
		/// <summary>
		/// Destroys all swap-chain image views.
		/// </summary>
		void destroyChainImageViews() noexcept
		{
			for(std::size_t i { 0U }; i < chain_views.size(); ++i)
			{
				destroyImageView(chain_views[i]);
			}
		}

		// ------------------------------------------------------------------ //
		// Helpers.
		// ------------------------------------------------------------------ //
		private:
		/// <summary>
		/// Ensures surface format is available by verifying its availability and
		/// fall back to an exiting one if not available.
		/// </summary>
		void ensureSurfaceFormat()
		{
			std::uint32_t n_formats { 0U };
			std::vector<vk::SurfaceFormatKHR> formats {};
			{
				vk::Result result { vk::Result::eSuccess };
				result = physical_device.getSurfaceFormatsKHR(
					surface, &n_formats, nullptr, dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "getSurfaceFormatsKHR"); }
				formats.resize(n_formats);
				result = physical_device.getSurfaceFormatsKHR(
					surface, &n_formats, formats.data(), dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "getSurfaceFormatsKHR"); }
			}
			for(std::uint32_t i { 0U }; i < n_formats; ++i)
			{
				bool const has_format {
					formats[i].format == info.surface_format.format };
				bool const has_colour {
					formats[i].colorSpace == info.surface_format.colorSpace };
				if(has_format && has_colour) { return; }
			}
			info.surface_format = formats[0U];
		}
		/// <summary>
		/// Ensures present mode is available by verifying its availability and
		/// fall back to an exiting one if not available.
		/// </summary>
		void ensurePresentMode()
		{
			std::uint32_t n_modes { 0U };
			std::vector<vk::PresentModeKHR> modes {};
			{
				vk::Result result { vk::Result::eSuccess };
				result = physical_device.getSurfacePresentModesKHR(
					surface, &n_modes, nullptr, dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "getSurfacePresentModesKHR"); }
				modes.resize(n_modes);
				result = physical_device.getSurfacePresentModesKHR(
					surface, &n_modes, modes.data(), dispatch);
				if(result != vk::Result::eSuccess)
				{ vk::throwResultException(result, "getSurfacePresentModesKHR"); }
			}
			for(std::uint32_t i { 0U }; i < n_modes; ++i)
			{
				if(modes[i] == info.present_mode) { return; }
			}
			info.present_mode = modes[0U];
		}
		/// <summary>
		/// Ensures the best extent possible.
		/// </summary>
		void ensureExtent(vk::SurfaceCapabilitiesKHR const & capabilities) noexcept
		{
			if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{ extent = capabilities.currentExtent; }
			else
			{
				extent.width = std::max(
					capabilities.minImageExtent.width,
					std::min(capabilities.maxImageExtent.width, extent.width));
				extent.height = std::max(
					capabilities.minImageExtent.height,
					std::min(capabilities.maxImageExtent.height, extent.height));
			}
		}
		/// <summary>
		/// Selects the minimum swap chain images.
		/// </summary>
		void selectMinimumChainImages(vk::SurfaceCapabilitiesKHR const & capabilities,
			std::uint32_t & minimum_images) const noexcept
		{
			minimum_images = capabilities.minImageCount + 1U;
			if(capabilities.maxImageCount > 0U)
			{
				if(minimum_images > capabilities.maxImageCount)
				{ minimum_images = capabilities.maxImageCount; }
			}
		}

		// ------------------------------------------------------------------ //
		// Obtain information.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Obtains the list of physical devices which are fit for the app.
		/// </summary>
		SwapChainInfo const & getChainInfo() const noexcept
		{ return info; }
	};
}

#endif
