#include <globalFuns.h>
#include <globalParams.h>

using namespace std;

/* Reads spv Files into RAM for GraphicsPipeline */
vector<char> pje::readSpirvFile(const string& filename) {
	// using ios:ate FLAG to retrieve fileSize => ate sets pointer to the end (ate => at end) ; _Prot default is set to 64
	ifstream currentFile(filename, ios::binary | ios::ate);

	// ifstream converts object currentFile to TRUE if the file was opened successfully
	if (currentFile) {
		// size_t guarantees to hold any array index => here tellg and ios::ate helps to get the size of the file
		size_t currentFileSize = (size_t)currentFile.tellg();
		vector<char> buffer(currentFileSize);

		// sets reading head to the beginning of the file
		currentFile.seekg(0);
		// reads ENTIRE binary into RAM for the program
		currentFile.read(buffer.data(), currentFileSize);
		// close will be called by destructor at the end of the scope but this line helps to understand the code
		currentFile.close();

		return buffer;
	}
	else {
		throw runtime_error("Failed to read shaderfile into RAM!");
	}
}

// ################################################################################################################################################################## //

/* Transforming shader code of type vector<char> into VkShaderModule */
void pje::createShaderModule(const vector<char>& spvCode, VkShaderModule& shaderModule) {
	VkShaderModuleCreateInfo shaderModuleInfo;
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.pNext = nullptr;
	shaderModuleInfo.flags = 0;
	shaderModuleInfo.codeSize = spvCode.size();
	shaderModuleInfo.pCode = (uint32_t*)spvCode.data();	// spv instructions always have a size of 32 bits, so casting is possible

	pje::context.result = vkCreateShaderModule(pje::context.logicalDevice, &shaderModuleInfo, nullptr, &shaderModule);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateShaderModule" << endl;
		throw runtime_error("Failed to create VkShaderModule");
	}
}

/* Adds element (module) to pje::shaderStageInfos */
void pje::addShaderModuleToShaderStages(VkShaderModule newModule, VkShaderStageFlagBits stageType, const char* shaderEntryPoint) {
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo;

	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.pNext = nullptr;
	shaderStageCreateInfo.flags = 0;
	shaderStageCreateInfo.stage = stageType;
	shaderStageCreateInfo.module = newModule;
	shaderStageCreateInfo.pName = shaderEntryPoint;			// entry point of the module
	shaderStageCreateInfo.pSpecializationInfo = nullptr;	// declaring const variables helps Vulkan to optimise shader code

	pje::context.shaderStageInfos.push_back(shaderStageCreateInfo);
}

/* Clears globalParam that will be requested by the GraphicsPipeline */
void pje::clearShaderStages() {
	pje::context.shaderStageInfos.clear();
}

// ################################################################################################################################################################## //

