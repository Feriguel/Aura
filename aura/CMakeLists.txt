# ============================================================================ #
# File: CMakeLists.txt [Aura]
#
# Description: Aura library. This contains all configurations needed to build
# base library and all the underlying modules.
#
# Author: Miguel Ângelo Crespo Ferreira
# ============================================================================ #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

# Sub-Libraries.
add_subdirectory("core")

# Create library.
add_library(Aura SHARED
	"source/engine.cpp"
)
add_library(Aura::Aura ALIAS Aura)
# Dependencies.
target_link_libraries(Aura
	PRIVATE
		Aura::Core
)

# ============================================================================ #
# Target Includes and features.
# ============================================================================ #
target_include_directories(Aura
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/source
)
