// ========================================================================== //
// File : engine.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/engine.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
// Standard includes.
#include <cstdint>
#include <string>
// External includes.

namespace Aura
{
	Engine::Engine(
		std::string const & app_name, std::uint16_t const & app_major,
		std::uint16_t const & app_minor, std::uint16_t const & app_patch
	) :
		core(new Core::Nucleus(app_name.c_str(), app_major, app_minor, app_patch))
	{}
	Engine::~Engine()
	{
		delete core;
	}
}
