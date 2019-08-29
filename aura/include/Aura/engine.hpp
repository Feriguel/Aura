// ========================================================================== //
// File : engine.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURA_ENGINE
#define AURA_ENGINE
// Internal includes.
#include <Aura/api.hpp>
// Standard includes.
#include <cstdint>
#include <string>
// External includes.

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
		class Nucleus;
	}
	/// <summary>
	/// Program engine starter.
	/// </summary>
	class Engine
	{
		private:
		Core::Nucleus * core;

		public:
		AURA_API_PUBLIC
			explicit Engine(
				std::string const & app_name, std::uint16_t const & app_major,
				std::uint16_t const & app_minor, std::uint16_t const & app_patch);
		AURA_API_PUBLIC
			~Engine();
		AURA_API_LOCAL
			constexpr Core::Nucleus * const getCore() noexcept
		{ return core; }
		AURA_API_PUBLIC
			constexpr Core::Nucleus const * const checkCore() const noexcept
		{ return core; }
	};
}

#endif
