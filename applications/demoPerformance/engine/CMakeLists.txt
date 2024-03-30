CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# adds additional sources to target (.exe || .lib)
target_sources(demoPerformance
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/engine_core.h"
)

# defines external dependencies
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(demoPerformance PRIVATE
	# external dependencies
	glm::glm
	assimp::assimp
	glfw
)