// ========================================================================== //
// File : environment.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/Environment/environment.hpp>
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
#include <cstdint>
#include <vector>
#include <limits>
// External includes.
#include <glm/glm.hpp>

namespace Aura::Core::Environment
{
	/// <summary>
	/// Adds a new transform to the list and return its index.
	/// </summary>
	[[nodiscard]]
	std::uint32_t Environment::addTransform() noexcept
	{
		std::uint32_t n_transforms = static_cast<std::uint32_t>( transforms.size() );
		if( n_transforms == Settings::limit_transforms )
		{ return std::numeric_limits<std::uint32_t>::max(); }
		transforms.push_back(Transform());
		return n_transforms;
	}
	/// <summary>
	/// Retrieves a non-constant transform at the given index.
	/// </summary>
	Transform * Environment::getTransform(std::uint32_t index) noexcept
	{
		std::uint32_t n_transforms = static_cast<std::uint32_t>( transforms.size() );
		if( index >= n_transforms )
		{ return nullptr; }
		return &transforms[index];
	}

	/// <summary>
	/// Adds a new camera to the list and return its index.
	/// </summary>
	[[nodiscard]]
	std::uint32_t Environment::addCamera() noexcept
	{
		std::uint32_t n_cameras = static_cast<std::uint32_t>( cameras.size() );
		if( n_cameras == Settings::limit_cameras )
		{ return std::numeric_limits<std::uint32_t>::max(); }
		cameras.push_back(Camera());
		return n_cameras;
	}
	/// <summary>
	/// Retrieves a non-constant camera.
	/// </summary>
	Camera * Environment::getCamera(std::uint32_t index) noexcept
	{
		std::uint32_t n_cameras = static_cast<std::uint32_t>( cameras.size() );
		if( index >= n_cameras )
		{ return nullptr; }
		return &cameras[index];
	}
}
