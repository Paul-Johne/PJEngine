CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# >> demo_lib.lib
target_sources(demo_lib
    PUBLIC
        # imgui - src
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets.cpp"
        # imgui - bindings
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_vulkan.cpp"

        # stb - src
        "${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.cpp"
)

target_include_directories(demo_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ImGui - find Vulkan and GLFW
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
target_link_libraries(demo_lib
    PRIVATE
        Vulkan::Vulkan
        glfw
)

# >> demoPerformance.lib
target_sources(demoPerformance_lib
    PUBLIC
        # stb - src
        "${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.cpp"
)

target_include_directories(demoPerformance_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)