CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(PJEngine)

# sets cpp version
#set(CMAKE_CXX_STANDARD 14)

# make additional CMake script visible (adds compile_shader.cmake functionality to CMake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# add subfolders containing additional CMakeLists.txt
add_subdirectory(applications)
add_subdirectory(libraries)