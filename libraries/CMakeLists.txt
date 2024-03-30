CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# generates .lib / .dll
# ==> used for target_link_libraries()
add_library(demo_lib "")
add_library(demoPerformance_lib "")

# adds sources to all targets (libraries)
add_subdirectory(static_libs)