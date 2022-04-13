CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(demo_lib)

set(DEMO_LIB_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/lib.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lib.cpp"
)

# generates .dll
add_library(demo_lib ${DEMO_LIB_SOURCES})

# enables <*.h> instead of using "*.h"
target_include_directories(demo_lib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")