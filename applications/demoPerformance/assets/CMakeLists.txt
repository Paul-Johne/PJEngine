CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# includes .cmake custom function
include(compile_shader)

# compiles shaders (for Vulkan)
set(SHADER_SOURCES_PERFORMANCE
	""
)
compile_shaders(
	demoPerformance_shaders
	SOURCES
		${SHADER_SOURCES_PERFORMANCE}
	OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
    COMPILE_OPTIONS
        "--target-env=vulkan1.0" "-O"
)

# ensures that demoPerformance_shaders is build before demoPerformance
add_dependencies(demoPerformance demoPerformance_shaders)

#add_custom_command()