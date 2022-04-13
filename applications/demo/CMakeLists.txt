CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

project(demo)

set(DEMO_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/app.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp"
)

# generates .exe
add_executable(demo ${DEMO_SOURCES})

# enables <*.h> instead of using "*.h"
target_include_directories(demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# define dependencies ; xxx::xxx => Vulkan::Vulkan
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui)

# ALSO linking libraries in libraries folder
target_link_libraries(demo PRIVATE
    # External Dependencies
    Vulkan::Vulkan
    glfw3::glfw3
    glm::glm
    imgui::imgui

    # Internal Dependencies
    demo_lib
)