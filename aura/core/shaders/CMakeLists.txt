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
		glslangValidator.exe -V ray-gen_f.comp -o ray-gen_f.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/ray-gen_f.spv
			${CMAKE_CURRENT_BINARY_DIR}/ray-gen_f.spv
	COMMAND
		glslangValidator.exe -V ray-gen_test.comp -o ray-gen_test.spv
	COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/ray-gen_test.spv
			${CMAKE_CURRENT_BINARY_DIR}/ray-gen_test.spv
	BYPRODUCTS
		ray-gen_f.spv ray-gen_test.spv
	COMMENT
		"Compiling shaders.."
	VERBATIM
)