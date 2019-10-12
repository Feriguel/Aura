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
		glslangValidator.exe -V pre-process.comp -o pre-process.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/pre-process.spv
			${CMAKE_CURRENT_BINARY_DIR}/pre-process.spv
	COMMAND
		glslangValidator.exe -V vertex.comp -o vertex.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/vertex.spv
			${CMAKE_CURRENT_BINARY_DIR}/vertex.spv
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
		glslangValidator.exe -V colour_and_scatter.comp -o colour_and_scatter.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/colour_and_scatter.spv
			${CMAKE_CURRENT_BINARY_DIR}/colour_and_scatter.spv
	COMMAND
		glslangValidator.exe -V post-process.comp -o post-process.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/post-process.spv
			${CMAKE_CURRENT_BINARY_DIR}/post-process.spv
	BYPRODUCTS
		pre-process.spv vertex.spv ray-gen.spv intersect.spv colour_and_scatter.spv post-process.spv
	COMMENT
		"Compiling shaders.."
	VERBATIM
)