CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(procedural_generation_with_vulkan)

# make additional CMake script visible (adds compile_shader.cmake functionality to CMake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# add subfolders containing additional CMakeLists.txt
add_subdirectory(applications)
add_subdirectory(libraries)