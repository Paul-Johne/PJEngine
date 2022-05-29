CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(demo)

set(DEMO_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/app.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp"
)

set(SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.frag"
)

# generates .exe
add_executable(demo ${DEMO_SOURCES})

# enables <*.h> instead of using "*.h"
target_include_directories(demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# defines dependencies ; xxx::xxx => Vulkan::Vulkan
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# linking libraries in libraries folder
target_link_libraries(demo PRIVATE
    # External Dependencies => Vulkan, GLFW and GLM
    # glfw3::glfw3 cannot find target => glfw for target glfw3
    Vulkan::Vulkan
    glfw
    glm::glm

    # Internal Dependencies => ImGui will be included directly with demo_lib
    demo_lib
)

# include .cmake file with additional functionality
include(compile_shader)

compile_shaders(
    demo_shaders
    ${SHADER_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
)