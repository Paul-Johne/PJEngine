/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>			// fixed size integer
	#include <string>			// std::string
	#include <array>			// std::array
	#include <vector>			// std::vector
	#include <memory>			// std::<smartPointer>

	#include <iostream>			// i/o stream
	#include <fstream>			// read from files
	#include <filesystem>		// file paths
	#include <chrono>			// time stamps
	#include <cmath>			// math functions
	#include <algorithm>		// classic functions for ranges

	#include <GL/gl3w.h>		// OpenGL [via gl3w]
	#include <vulkan/vulkan.h>	// Vulkan
	#include <glm/glm.hpp>		// glm
	#include <GLFW/glfw3.h>		// GLFW

/* Project Files */
	#include "engine/argsParser.h"
	#include "engine/lSysGenerator.h"
	#include "engine/turtleInterpreter.h"