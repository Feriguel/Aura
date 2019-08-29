// ========================================================================== //
// File : info.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_INFO
#define AURACORE_INFO
// Internal includes.
// Standard includes.
#include <cstdint>
#include <string>
// External includes.
#include <vulkan/vulkan.h>

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura
{
	/// <summary>
	/// Aura core namespace.
	/// </summary>
	namespace Core
	{
		/// <summary>
		/// Structure which contains the program basic information.
		/// </summary>
		struct Info
		{
			// Application or engine name.
			std::string name {};
			// Version major.
			std::uint16_t major {};
			// Version minor.
			std::uint16_t minor {};
			// Version patch.
			std::uint16_t patch {};
		};
		/// <summary>
		/// Builds the auto generated engine info structure.
		/// </summary>
		static inline Info const getEngineInfo() noexcept
		{
			return Info {"Aura", 0, 1, 0 };
		}
		/// <summary>
		/// Builds the vulkan version.
		/// </summary>
		static inline std::uint32_t const makeVulkanVersion(Info const & info) noexcept
		{
			return VK_MAKE_VERSION(info.major, info.minor, info.patch);
		}
	}
}

#endif
