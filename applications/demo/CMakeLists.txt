CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

#project(demo)

set(DEMO_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/app.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/debugUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/globalFuns.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/globalParams.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/modelloader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vef.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/globalFuns.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/globalParams.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vef.cpp"
)

set(SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.frag"
)

# generates demo.exe
add_executable(demo ${DEMO_SOURCES})

# changes Cpp for this target (top level already sets Cpp Version to 11)
set_property(
    TARGET 
        demo 
    PROPERTY
        CXX_STANDARD 17
)

# sets right value for __cplusplus parameter
if(MSVC)
    target_compile_options(demo PUBLIC "/Zc:__cplusplus")
endif()

# enables <*.h> instead of using "*.h"
target_include_directories(demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# defines dependencies ; xxx::xxx => Vulkan::Vulkan
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)

# linking libraries in libraries folder
target_link_libraries(demo PRIVATE
    # External Dependencies => Vulkan, GLFW, GLM and Assimp
    Vulkan::Vulkan
    glfw
    glm::glm
    assimp::assimp

    # Internal Dependencies => ImGui will be included directly via demo_lib
    demo_lib
)

# include .cmake file with additional functionality
include(compile_shader)

compile_shaders(
    demo_shaders
    SOURCES
        ${SHADER_SOURCES}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
    COMPILE_OPTIONS
        "--target-env=vulkan1.0" "-O"
)

add_dependencies(demo demo_shaders)

# add 3D objects to binary project =====> TODO (Choosing objects dynamically)
add_custom_command(
    TARGET demo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_3d.obj"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_3d.obj"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_3d.fbx"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_3d.fbx"
)