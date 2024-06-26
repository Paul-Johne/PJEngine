CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# adds additional sources to target (.exe || .lib)
target_sources(demoPerformance
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/pjeBuffers.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/pjeBuffers.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/argsParser.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/argsParser.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/lSysGenerator.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/lSysGenerator.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/sourceloader.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/sourceloader.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/turtleInterpreter.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/turtleInterpreter.cpp"
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