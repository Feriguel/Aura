// ========================================================================== //
// File : api.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURA_API
#define AURA_API

/// <summary>
/// Definitions to create shared libraries cross-plat.
/// </summary>
#if defined(_WIN32) || defined(__CYGWIN__)
// If CMake auto generated export flag is set.
#	if defined(Aura_EXPORTS)
#		if defined(__GNUC__)
///			<summary>
///			Public symbol, part of shared library API.
///			</summary>
#			define AURA_API_PUBLIC __attribute__ ((dllexport))
#		else
///			<summary>
///			Public symbol, part of shared library API.
///			</summary>
#			define AURA_API_PUBLIC __declspec(dllexport)
#		endif
//	If CMake auto generated export flag isn't set.
#	else
#		ifdef __GNUC__
///			<summary>
///			Public symbol, part of shared library API.
///			</summary>
#			define AURA_API_PUBLIC __attribute__ ((dllimport))
#		else
///			<summary>
///			Public symbol, part of shared library API.
///			</summary>
#			define AURA_API_PUBLIC __declspec(dllimport)
#		endif
#	endif
///		<summary>
///		Local symbol, not part of shared library API.
///		</summary>
#	define AURA_API_LOCAL
#else
#if __GNUC__ >= 4
///		<summary>
///		Public symbol, part of shared library API.
///		</summary>
#		define AURA_API_PUBLIC __attribute__ ((visibility ("default")))
///		<summary>
///		Local symbol, not part of shared library API.
///		</summary>
#		define AURA_API_LOCAL  __attribute__ ((visibility ("hidden")))
#	else
///		<summary>
///		Public symbol, part of shared library API.
///		</summary>
#		define AURA_API_PUBLIC
///		<summary>
///		Local symbol, not part of shared library API.
///		</summary>
#		define AURA_API_LOCAL
#	endif
#endif

#endif
