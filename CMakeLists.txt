CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(PJEngine)

# make additional CMake script visible
# ==> adds compile_shader.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# add subfolders containing additional CMakeLists.txt
add_subdirectory(libraries)
add_subdirectory(applications)