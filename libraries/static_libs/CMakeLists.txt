CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# >> demo_lib.lib
target_sources(demo_lib
    PUBLIC
        # imgui - include folder
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imconfig.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_internal.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_rectpack.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_textedit.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imstb_truetype.h"
        # imgui - binding headers
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_glfw.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui_impl_vulkan.h"

        # stb - include folder
        "${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image.h"

        # imgui - src folder
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets.cpp"
        # imgui - binding code
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_impl_vulkan.cpp"

        # src folder - stb
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
        # stb files
        "${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.cpp"
)

target_include_directories(demoPerformance_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)