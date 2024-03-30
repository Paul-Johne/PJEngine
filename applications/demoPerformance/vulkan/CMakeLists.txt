CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# defines external dependencies
find_package(Vulkan REQUIRED)

target_link_libraries(demoPerformance PRIVATE
	# external dependencies
	Vulkan::Vulkan
)