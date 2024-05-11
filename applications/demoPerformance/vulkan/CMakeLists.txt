CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# adds additional sources to target (.exe || .lib)
target_sources(demoPerformance
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/rendererVK.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/rendererVK.cpp"
)

# defines external dependencies
find_package(Vulkan REQUIRED)

target_link_libraries(demoPerformance PRIVATE
	# external dependencies
	Vulkan::Vulkan
)