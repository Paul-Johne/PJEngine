/* Ignores header if not needed after #include */
	#pragma once

/* enables code in header files */
	//#define VK_USE_PLATFORM_WIN32_KHR 1	
	//#define GLFW_INCLUDE_VULKAN

	#include <cstdint>
	#include <iostream>
	#include <fstream>

	#include <string>
	#include <array>
	#include <vector>
	#include <limits>

	#include <vulkan/vulkan.h>
	#include <GLFW/glfw3.h>
	#include <glm/glm.hpp>
	#include <imgui.h>

	#include <globalParams.h>
	#include <globalFuns.h>
	#include <vef.h>
	#include <debugUtils.h>