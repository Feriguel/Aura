# ============================================================================ #
# File: CMakeLists.txt [Main]
#
# Description: Project global configurations and included sub-libraries and
# dependencies.
#
# Author: Miguel Ângelo Crespo Ferreira
# ============================================================================ #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

# Global project information.
project ("Aura"	
	VERSION 0.1.0
	DESCRIPTION "A ray-tracer using compute shaders and Vulkan"
	HOMEPAGE_URL "https://github.com/Feriguel/Aura"
	LANGUAGES CXX
)

# Project options.
option(AURA_BUILD_TESTS "Build tests" ON)

# Global external package dependencies.
find_package(Vulkan 1.1 REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

# Library.
add_subdirectory("aura")
# Tests.
if(AURA_BUILD_TESTS)
	enable_testing()
	find_package(GTest MODULE REQUIRED)
	add_subdirectory("tests")
	add_test(
		NAME
			Test
		COMMAND
			Test
	)
endif()