/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>							// fixed size integer
	#include <string>							// std::string
	#include <memory>							// std::<smartPointer>
	#include <unordered_map>					// hashtable

	#include <iostream>							// i/o stream
	#include <stdexcept>						// std::runtime_error
	#include <chrono>							// time stamps
	#include <algorithm>						// classic functions for ranges
	#include <execution>						// parallel algorithms

	#include <assimp/scene.h>					// Assimp: data structure
	#include <assimp/Importer.hpp>				// Assimp: importer interface
	#include <assimp/postprocess.h>				// Assimp: post processing flags
	#include <stb_image.h>						// stb
	#include <GL/gl3w.h>						// OpenGL [via gl3w]
	#include <vulkan/vulkan.h>					// Vulkan
	#include <glm/glm.hpp>						// glm types
	#include <glm/gtx/string_cast.hpp>			// glm to string debugging
	#include <glm/gtc/matrix_transform.hpp>		// glm matrix operations
	#include <GLFW/glfw3.h>						// GLFW

/* Project Files */
	#include "engine/pjeBuffers.h"
	#include "engine/argsParser.h"
	#include "engine/lSysGenerator.h"
	#include "engine/sourceloader.h"
	#include "engine/turtleInterpreter.h"
	#include "opengl/rendererGL.h"
	#include "vulkan/rendererVK.h"

double getMedian(std::vector<size_t> dataset) {
	auto n = dataset.size();

	if (n % 2 == 0) {
		std::nth_element(dataset.begin(), dataset.begin() + n / 2, dataset.end());
		std::nth_element(dataset.begin(), dataset.begin() + (n - 1) / 2, dataset.end());

		return (dataset[(n / 2)] + dataset[(n - 1) / 2]) / 2.0;
	}
	else {
		std::nth_element(dataset.begin(), dataset.begin() + n / 2, dataset.end());

		return dataset[n / 2];
	}
}

double getStandardDeviation(std::vector<size_t> dataset, double mean) {
	double sd = 0.0;

	for (const auto& element : dataset) {
		sd += pow(element - mean, 2);
	}
	return sqrt(sd / dataset.size());
}