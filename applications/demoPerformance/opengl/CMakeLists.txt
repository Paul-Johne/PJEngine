CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# adds additional sources to target (.exe || .lib)
target_sources(demoPerformance
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/rendererGL.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/rendererGL.cpp"
)

# defines external dependencies
find_package(gl3w REQUIRED)

target_link_libraries(demoPerformance PRIVATE
	# external dependencies
	gl3w
)