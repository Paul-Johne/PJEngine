CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# defines external dependencies
find_package(gl3w REQUIRED)

target_link_libraries(demoPerformance PRIVATE
	# external dependencies
	gl3w
)