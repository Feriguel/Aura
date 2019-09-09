# ============================================================================ #
# File: CMakeLists.txt [Core]
#
# Description: Aura library core component. Main component of the library, which
# controls the entire engine.
#
# Author: Miguel Ângelo Crespo Ferreira
# ============================================================================ #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

# Create library.
add_library(Core STATIC
	"source/nucleus.cpp"
	"source/ui.cpp"
	"source/environment.cpp"
	"source/render.cpp"
	"source/Render/ray-tracer.cpp"
)
add_library(Aura::Core ALIAS Core)
# Add subfolders.
add_subdirectory("shaders")
# Add dependencies.
add_dependencies(Core Shaders)
# Link external libraries.
target_link_libraries(Core
	PUBLIC
		glm Vulkan::Vulkan
	PRIVATE
		glfw
)
target_link_options(Core
	PRIVATE
		-static glfw
)
# Configures a header file with version numbers for internal use.
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/info.hpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Aura/Core/Utilities/info.hpp"
)
# ============================================================================ #
# Target Includes and features.
# ============================================================================ #
target_include_directories(Core
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/source
)
target_compile_features(Core PUBLIC cxx_std_20)