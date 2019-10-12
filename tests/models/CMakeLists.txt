# ============================================================================ #
# File: CMakeLists.txt [Scenes]
#
# Description: Examples configurations, this is loaded if AURA_BUILD_EXAMPLES is
# ON.
#
# Author: Miguel Ângelo Crespo Ferreira
# =========================================================================== #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/suzanne.obj"
	"${CMAKE_CURRENT_BINARY_DIR}/suzanne.obj"
	COPYONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cube.obj"
	"${CMAKE_CURRENT_BINARY_DIR}/cube.obj"
	COPYONLY
)
