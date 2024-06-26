CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

set(DEMO_SOURCES
    ### Headers ###
    "${CMAKE_CURRENT_SOURCE_DIR}/include/app.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/globalFuns.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/modelloader.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/globalParams.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/debugUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vef.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/engineGui.h"
    ### Source Files ###
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/globalFuns.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modelloader.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/globalParams.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vef.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/engineGui.cpp"
)

set(SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic.frag"
)

# generates demo.exe
add_executable(demo ${DEMO_SOURCES})

# changes Cpp for this target
set_property(
    TARGET 
        demo 
    PROPERTY
        CXX_STANDARD 17
)

# sets right value for __cplusplus parameter
if(MSVC)
    target_compile_options(demo PRIVATE "/Zc:__cplusplus")
endif()

# enables <*.h> instead of using "*.h"
# IMPORTANT : use command AFTER adding all sources to target
target_include_directories(demo 
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

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

    # Internal Dependencies => ImGui and stb
    demo_lib
)

# include .cmake file with additional functionality
include(compile_shader)

# provided by Fabian Friederichs
compile_shaders(
    demo_shaders
    SOURCES
        ${SHADER_SOURCES}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
    COMPILE_OPTIONS
        "--target-env=vulkan1.0" "-O"
)

# ensures that demo_shader is build before demo
add_dependencies(demo demo_shaders)

# add 3D objects to binary project =====> TODO (Choosing objects dynamically)
add_custom_command(
    TARGET demo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_3d.obj"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_3d.obj"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_3d.mtl"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_3d.mtl"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_3d.fbx"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_3d.fbx"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_chilli_plant.obj"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_chilli_plant.obj"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_chilli_plant.mtl"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_chilli_plant.mtl"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/pje_chilli_plant.fbx"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/pje_chilli_plant.fbx"
    # Temporary until embedded textures are fixed
    # UPDATE : not needed anymore
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/chilli_plant_albedo_depth8.png"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/chilli_plant_albedo_depth8.png"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models/debug.jpg"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/models/debug.jpg"
)