CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# includes .cmake custom function
include(compile_shader)

# compiles shaders (Vulkan-specific)
set(SHADER_SOURCES_PERFORMANCE
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic_vulkan.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic_vulkan.frag"
)
compile_shaders(
	target_vulkanShaders
	SOURCES
		${SHADER_SOURCES_PERFORMANCE}
	OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMPILE_OPTIONS
        "--target-env=vulkan1.0" "-O"
)

# ensures that target_vulkanShaders is build before demoPerformance
add_dependencies(demoPerformance target_vulkanShaders)

# copies all primitives into build folder
add_custom_target(target_copyPrimitives
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/primitives/pje_plant_ground.fbx"
		"${CMAKE_CURRENT_BINARY_DIR}/primitives/pje_plant_ground.fbx"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/primitives/pje_plant_stem.fbx"
		"${CMAKE_CURRENT_BINARY_DIR}/primitives/pje_plant_stem.fbx"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/primitives/pje_plant_leaf.fbx"
		"${CMAKE_CURRENT_BINARY_DIR}/primitives/pje_plant_leaf.fbx"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/primitives/pje_plant_flower.fbx"
		"${CMAKE_CURRENT_BINARY_DIR}/primitives/pje_plant_flower.fbx"
	COMMENT
		"[target_copyPrimitives]: Copying primitives to build folder - done"
)

# ensures that target_copyPrimitives is build before demoPerformance
add_dependencies(demoPerformance target_copyPrimitives)

# copies all OpenGL-shader into build folder (OpenGL-specific)
add_custom_target(target_openglShaders
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic_opengl.vert"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/basic_opengl.vert"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic_opengl.frag"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/basic_opengl.frag"
	COMMENT
		"[target_openglShaders]: Copying raw OpenGL shaders to build folder - done"
)

# ensures that target_openglShaders is build before demoPerformance
add_dependencies(demoPerformance target_openglShaders)