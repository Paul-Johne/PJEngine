CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# project name used for target_link_libraries() in application
project(demo_lib)

# declares all the sources
set(DEMO_LIB_SOURCES
    # include folder
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imconfig.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_internal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_rectpack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_textedit.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_truetype.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_glfw.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_vulkan.h"

    # src folder
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.cpp"
    #"${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_vulkan.cpp"
)

# generates .dll
add_library(demo_lib ${DEMO_LIB_SOURCES})

# enables <*.h> instead of using "*.h"
target_include_directories(demo_lib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# find Vulkan and GLFW for ImGui
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(demo_lib PRIVATE
    Vulkan::Vulkan
    glfw
)