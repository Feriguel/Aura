// ========================================================================== //
// File : environment.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_ENV
#define AURACORE_ENV
// Internal includes.
#include <Aura/Core/types.hpp>
#include <Aura/Core/settings.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
#include <cstdint>
#include <vector>
// External includes.

/// <summary>
/// Aura core environment namespace.
/// </summary>
namespace Aura::Core::Environment
{
	/// <summary>
	/// Contains all the environment structure which compose the current
	/// scene.
	/// </summary>
	class Environment
	{
		std::vector<Transform> transforms;
		std::vector<Camera> cameras;

		public:
		/// <summary>
		/// Adds a new transform to the list and return its index.
		/// </summary>
		/// <returns> New transform's index. </returns>
		std::uint32_t addTransform() noexcept;
		/// <summary>
		/// Retrieves a non-constant transform at the given index.
		/// </summary>
		/// <param name="index"> Index of transform to retrieve. </param>
		/// <returns> Transform pointer or null pointer. </returns>
		Transform * getTransform(std::uint32_t index) noexcept;
		/// <summary>
		/// Adds a new transform to the list and return its index.
		/// </summary>
		/// <returns> New transform's index. </returns>
		std::uint32_t addCamera() noexcept;
		/// <summary>
		/// Retrieves a non-constant camera.
		/// </summary>
		/// <param name="index"> Index of camera to retrieve. </param>
		/// <returns> Camera pointer or null pointer. </returns>
		Camera * getCamera(std::uint32_t index) noexcept;

		//----------------------------------------------------------------------
		// Constant functions.
		//----------------------------------------------------------------------
		/// <summary>
		/// Retrieves a constant list of all transforms.
		/// </summary>
		constexpr std::vector<Transform> const & getTransformsState() const noexcept
		{ return transforms; }
		/// <summary>
		/// Retrieves a constant list of all cameras.
		/// </summary>
		constexpr std::vector<Camera> const & getCamerasState() const noexcept
		{ return this->cameras; }
	};
}

#endif
