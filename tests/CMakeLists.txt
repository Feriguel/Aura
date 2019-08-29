# ============================================================================ #
# File: CMakeLists.txt [Tests]
#
# Description: Test configurations, this is loaded if AURA_BUILD_TESTS is ON.
#
# Author: Miguel Ângelo Crespo Ferreira
# ============================================================================ #

cmake_minimum_required (VERSION 3.14...3.15 FATAL_ERROR)

# Example executable.
add_executable(Test 
	"source/main.cpp"
)
add_executable(Aura::Test ALIAS Test)
# Dependencies.
target_link_libraries(Test 
	PRIVATE
		Aura::Core GTest::GTest GTest::Main Vulkan::Vulkan glfw glm 
)
# ============================================================================ #
# Target Includes and features.
# ============================================================================ #
target_include_directories(Test
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/source
)
target_compile_features(Test PUBLIC cxx_std_20)