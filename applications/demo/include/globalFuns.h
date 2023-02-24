/* Ignores header if not needed after #include */
	#pragma once

	#include <iostream>
	#include <fstream>
	#include <string>
	#include <vector>
	#include <chrono>

	#include <vulkan/vulkan.h>
	#include <glm/gtc/matrix_transform.hpp>

	#include <globalParams.h>
	#include <vef.h>
	#include <debugUtils.h>

namespace pje {
	std::vector<char> readSpirvFile(const std::string& filename);

	void createShaderModule(const std::vector<char>& spvCode, VkShaderModule& shaderModule);
	void addShaderModuleToShaderStages(VkShaderModule newModule, VkShaderStageFlagBits stageType, const char* shaderEntryPoint = "main");

	void cleanupRealtimeRendering(bool reset = false);

	int startGlfw3(const char* windowName);
	void stopGlfw3();

	int startVulkan();
	void stopVulkan();

	void loopVisualizationOf(GLFWwindow* window);
}

void setObjectNames(const pje::Context& context);