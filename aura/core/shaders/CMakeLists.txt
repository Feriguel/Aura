# ============================================================================ #
# File: CMakeLists.txt [Shaders]
#
# Description: Sets the shaders for automatic compilation and shipping to the
# intended folder.
#
# Author: Miguel Ângelo Crespo Ferreira
# ============================================================================ #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

# Compile and ship shaders.
add_custom_target(Shaders
	WORKING_DIRECTORY
		"${CMAKE_CURRENT_SOURCE_DIR}"
	COMMAND
		glslangValidator.exe -V ray-gen.comp -o ray-gen.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/ray-gen.spv
			${CMAKE_CURRENT_BINARY_DIR}/ray-gen.spv
	BYPRODUCTS
		ray-gen.spv
	COMMENT
		"Compiling shaders.."
	VERBATIM
)