CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# generates demoPerformance.exe
set(DEMO_PERFORMANCE_SOURCES 
	"${CMAKE_CURRENT_SOURCE_DIR}/app.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/app.cpp"
)
add_executable(demoPerformance ${DEMO_PERFORMANCE_SOURCES})

# sets C++ version for .exe
set_property(
	TARGET
		demoPerformance
	PROPERTY
		CXX_STANDARD 17
)

# sets __cplusplus value
if(MSVC)
	target_compile_options(demoPerformance PRIVATE "/Zc:__cplusplus")
endif()

# adds engine core to .exe
add_subdirectory(engine)

# adds graphics APIs to .exe
add_subdirectory(vulkan)
add_subdirectory(opengl)

# adds assets to .exe
add_subdirectory(assets)

target_link_libraries(demoPerformance PRIVATE
	# internal dependencies
	demoPerformance_lib
)