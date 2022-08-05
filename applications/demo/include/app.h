/* Ignores header if not needed after #include */
#pragma once

/* enables (sets to 1) code in header files*/
//#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR 1

/* Including other needs */
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <limits>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

//#include "./../../../libraries/demo_lib/include/imgui.h"
#include <imgui.h>