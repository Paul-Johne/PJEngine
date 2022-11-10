/* Ignores header if not needed after #include */
	#pragma once

	#include <iostream>
	#include <fstream>

	#include <string>
	#include <vector>
	#include <vulkan/vulkan.h>

namespace pje {
	std::vector<char> readSpirvFile(const std::string& filename);

	void createShaderModule(const std::vector<char>& spvCode, VkShaderModule& shaderModule);
	void addShaderModuleToShaderStages(VkShaderModule newModule, VkShaderStageFlagBits stageType, const char* shaderEntryPoint = "main");
	void clearShaderStages();
}