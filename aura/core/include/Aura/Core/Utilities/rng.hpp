// ========================================================================== //
// File : rng.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_RNG
#define AURACORE_RNG
// Internal includes.
// Standard includes.
#include <random>
// External includes.

/// <summary>
/// Aura main namespace.
/// </summary>
namespace Aura
{
	/// <summary>
	/// Aura core environment namespace.
	/// </summary>
	namespace Core
	{
		/// <summary>
		/// God of entropy and unpredictability.
		/// </summary>
		/// 
		class RNGesus
		{
			// Random number generator, used in number retrieval form the distribution.
			std::mt19937 random_number_generator;
			// Uniform distribution of real numbers between [-1, 1[.
			std::uniform_real_distribution<float> distribution;

			// ------------------------------------------------------------------ //
			// Set-up.
			// ------------------------------------------------------------------ //
			public:
			/// <summary>
			/// Sets-up the random number generator and the distribution of the current float type.
			/// </summary>
			RNGesus() :
				random_number_generator(randomSeed()),
				distribution(-1.0f, 1.0f)
			{}

			// ------------------------------------------------------------------ //
			// Random number generation.
			// ------------------------------------------------------------------ //
			public:
			/// <summary>
			/// Generate a random number between 0 and 1.
			/// </summary>
			float gen()
			{ return distribution(this->random_number_generator); }

			// ------------------------------------------------------------------ //
			// Generator retrieval.
			// ------------------------------------------------------------------ //
			private:
			/// <summary>
			/// Retrieve a seed from the hardware.
			/// </summary>
			std::random_device::result_type randomSeed() const
			{
				std::random_device random {};
				return random();
			}
		};
	}
}

#endif
