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
	COMMAND
		glslangValidator.exe -V intersect.comp -o intersect.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/intersect.spv
			${CMAKE_CURRENT_BINARY_DIR}/intersect.spv
	COMMAND
		glslangValidator.exe -V colour.comp -o colour.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/colour.spv
			${CMAKE_CURRENT_BINARY_DIR}/colour.spv
	COMMAND
		glslangValidator.exe -V scatter.comp -o scatter.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/scatter.spv
			${CMAKE_CURRENT_BINARY_DIR}/scatter.spv
	COMMAND
		glslangValidator.exe -V post-process.comp -o post-process.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/post-process.spv
			${CMAKE_CURRENT_BINARY_DIR}/post-process.spv
	BYPRODUCTS
		ray-gen.spv intersect.spv colour.spv scatter.spv post-process.spv
	COMMENT
		"Compiling shaders.."
	VERBATIM
)