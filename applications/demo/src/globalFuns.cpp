#include <globalFuns.h>

using namespace std;

/* Reads spv Files into RAM */
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

/* Transforming shader code into VkShaderModule */
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

/* Adds VkShaderModule to pje::shaderStageInfos */
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

/* Clears pje::context.shaderStageInfos */
void clearShaderStages() {
	pje::context.shaderStageInfos.clear();
}

// ################################################################################################################################################################## //

/* Searches for an memory type index of the choosen physical device that equals memoryTypeBits and the given flags*/
uint32_t getMemoryTypeIndex(uint32_t memoryTypeBits, VkMemoryPropertyFlags flags) { 
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(pje::context.physicalDevices[pje::context.choosenPhysicalDevice], &memProps);

	// Compares all GPU's valid memory types with given requirements
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & flags) == flags) {
			return i;
		}
	}

	throw runtime_error("Error at getMemoryTypeIndex");
}

/* Returns a VkBuffer Handle withpout VkMemory */
VkBuffer createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, const char* debugName = nullptr) {
	VkBuffer buffer = VK_NULL_HANDLE;
	
	VkBufferCreateInfo vkBufferInfo;
	vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferInfo.pNext = nullptr;
	vkBufferInfo.flags = 0;
	vkBufferInfo.size = size;
	vkBufferInfo.usage = usageFlags;
	vkBufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	vkBufferInfo.queueFamilyIndexCount = 0;			// while VK_SHARING_MODE_EXCLUSIVE
	vkBufferInfo.pQueueFamilyIndices = nullptr;		// while VK_SHARING_MODE_EXCLUSIVE

	pje::context.result = vkCreateBuffer(pje::context.logicalDevice, &vkBufferInfo, nullptr, &buffer);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateBuffer" << endl;
		throw runtime_error("Error at vkCreateBuffer");
	}

#ifndef NDEBUG
	if (debugName) {
		/* buffer is a handle and will exist after completing this function */
		pje::set_object_name(pje::context.logicalDevice, buffer, debugName);
	}
#endif

	return buffer;
}

/* Creates VkMemory for a given VkBuffer */
VkDeviceMemory allocateVkBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags memoryFlags) {
	VkDeviceMemory memory = VK_NULL_HANDLE;

	// Calculates the size the given VkBuffer requires
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(pje::context.logicalDevice, buffer, &memReq);

	VkMemoryAllocateInfo memAllocInfo;
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = memReq.size;
	memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReq.memoryTypeBits, memoryFlags);

	pje::context.result = vkAllocateMemory(pje::context.logicalDevice, &memAllocInfo, nullptr, &memory);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkAllocateMemory" << endl;
		throw runtime_error("Error at vkAllocateMemory");
	}

	return memory;
}

/* Creates and binds VkBuffer and Memory of some pje::PJBuffer */
void createPJBuffer(pje::PJBuffer& rawPJBuffer, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, const char* debugName = nullptr) {
	rawPJBuffer.buffer = createVkBuffer(size, usageFlags, debugName);
	rawPJBuffer.size = size;
	
	rawPJBuffer.deviceMemory = allocateVkBufferMemory(rawPJBuffer.buffer, memoryFlags);
	rawPJBuffer.flags = memoryFlags;

	vkBindBufferMemory(pje::context.logicalDevice, rawPJBuffer.buffer, rawPJBuffer.deviceMemory, 0);
}

/* Maintains pje::stagingBuffer and writes void* data into local device VkBuffer dst */
void copyToLocalDeviceBuffer(const void* data, VkDeviceSize dataSize, VkBuffer dst, VkDeviceSize dstOffset = 0) {
	if (pje::stagingBuffer.buffer == VK_NULL_HANDLE || pje::stagingBuffer.size < dataSize) {
		if (pje::stagingBuffer.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(pje::context.logicalDevice, pje::stagingBuffer.buffer, nullptr);
			vkFreeMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory, nullptr);
		}

		createPJBuffer(
			pje::stagingBuffer,
			dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"stagingBuffer"
		);
	}

	/* Mapping of void* data to pje::stagingBuffer */
	void* stagingPointer;
	// VK_WHOLE_SIZE => whole size is mappable
	vkMapMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory, 0, VK_WHOLE_SIZE, 0, &stagingPointer);
	memcpy(stagingPointer, data, dataSize);
	// vkFlushMappedMemoryRange() || VK_MEMORY_PROPERTY_HOST_COHERENT_BIT == 1
	vkUnmapMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory);

	static VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;	// static variable will be created once for all executions of the given function

	if (stagingCommandBuffer == VK_NULL_HANDLE) {
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.commandPool = pje::context.commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, &stagingCommandBuffer);
	}
	else {
		vkResetCommandBuffer(stagingCommandBuffer, 0);
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // ensures driver that vkSubmit will only happen ONCE
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	/* START - RECORDING */
	vkBeginCommandBuffer(stagingCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy bufferCopyRegion1;
	bufferCopyRegion1.srcOffset = 0;
	bufferCopyRegion1.dstOffset = dstOffset;
	bufferCopyRegion1.size = dataSize;
	vkCmdCopyBuffer(stagingCommandBuffer, pje::stagingBuffer.buffer, dst, 1, &bufferCopyRegion1);

	vkEndCommandBuffer(stagingCommandBuffer);
	/* END - RECORDING */

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stagingCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	/* Submission of VkCommandBuffer onto VkQueue */
	vkQueueSubmit(pje::context.queueForPrototyping, 1, &submitInfo, pje::context.fenceCopiedBuffer);

	vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceCopiedBuffer, VK_TRUE, numeric_limits<uint64_t>::max());
	vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceCopiedBuffer);
}

/* Sends data on RAM as PJBuffer to [VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT]-VRAM */
template <typename T>
void sendDataToVRAM(pje::PJBuffer& localDeviceBuffer, vector<T> objectTarget, VkBufferUsageFlags vertexOrIndexFlag, const char* debugName = nullptr) {
	VkDeviceSize objectByteSize = sizeof(T) * objectTarget.size();

	createPJBuffer(
		localDeviceBuffer,
		objectByteSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | vertexOrIndexFlag,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		debugName
	);

	// HOST_COHERENT buffer -> DEVICE_LOCAL buffer
	copyToLocalDeviceBuffer(objectTarget.data(), objectByteSize, localDeviceBuffer.buffer);
}

// ################################################################################################################################################################## //

/* Defines the Layout of VkDescriptorSet which declares needed data in each shader */
void createDescriptorSetLayout(VkDescriptorSetLayout& rawLayout) {
	// SLOT (or binding) of VkDescriptorSetLayout
	array<VkDescriptorSetLayoutBinding, 3> layoutBindings;
							
	layoutBindings[0].binding = 0;																// layout(binding = n)
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;														// in shader code behind one binding => array[descriptorCount]
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindings[0].pImmutableSamplers = nullptr;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[1].pImmutableSamplers = nullptr;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[2].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.bindingCount = layoutBindings.size();
	layoutCreateInfo.pBindings = layoutBindings.data();

	pje::context.result = vkCreateDescriptorSetLayout(pje::context.logicalDevice, &layoutCreateInfo, nullptr, &rawLayout);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateDescriptorSetLayout" << endl;
		throw runtime_error("Error at vkCreateDescriptorSetLayout");
	}
}

/* Creates a VkDescriptorPool whose VkDescritorSet-items have 1 uniform and 2 storage buffers */
void createDescriptorPool(VkDescriptorPool& rawPool) {
	auto poolSizes = array{														// TODO (should be defined outside of function for a better abstraction)
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
	};

	VkDescriptorPoolCreateInfo poolCreateInfo;
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.pNext = nullptr;
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets = 1;													// allocatable descriptor sets
	poolCreateInfo.poolSizeCount = poolSizes.size();
	poolCreateInfo.pPoolSizes = poolSizes.data();

	pje::context.result = vkCreateDescriptorPool(pje::context.logicalDevice, &poolCreateInfo, nullptr, &rawPool);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateDescriptorPool" << endl;
		throw runtime_error("Error at vkCreateDescriptorPool");
	}
}

/* Creates VkDescriptorSet from context.descriptorPool regarding context.descriptorSetLayout */
void createDescriptorSet(VkDescriptorSet &rawDescriptorSet) {
	VkDescriptorSetAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.descriptorPool = pje::context.descriptorPool;
	allocateInfo.descriptorSetCount = 1;							// number of DescriptorSets of this DescriptorSet "type"
	allocateInfo.pSetLayouts = &pje::context.descriptorSetLayout;

	pje::context.result = vkAllocateDescriptorSets(pje::context.logicalDevice, &allocateInfo, &rawDescriptorSet);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkAllocateDescriptorSets" << endl;
		throw runtime_error("Error at vkAllocateDescriptorSets");
	}
}

/* Links uniform buffer or storage buffer to an existing VkDescriptorSet */
void linkDescriptorSetToBuffer(VkDescriptorSet& descriptorSet, uint32_t dstBinding, VkDescriptorType type, pje::PJBuffer& buffer, VkDeviceSize sizeofData = VK_WHOLE_SIZE) {
	/* Telling descriptorSet what it is suppose to contain */
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;							// buffer content on RAM
	bufferInfo.offset = 0;
	bufferInfo.range = sizeofData;

	/* Updating descriptors of descriptorSet */
	VkWriteDescriptorSet update;
	update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	update.pNext = nullptr;
	update.dstSet = descriptorSet;
	update.dstBinding = dstBinding;
	update.dstArrayElement = 0;									// first element		in array[descriptorCount]
	update.descriptorCount = 1;									// amount of elements	in array[descriptorCount]
	update.descriptorType = type;
	update.pImageInfo = nullptr;
	update.pBufferInfo = &bufferInfo;							// descriptor's buffer
	update.pTexelBufferView = nullptr;

	/* Descriptor/Pointer Update */
	vkUpdateDescriptorSets(pje::context.logicalDevice, 1, &update, 0, nullptr);
}

/* Create uniform by creating PJBuffer */
void createUniformBuffer(pje::PJBuffer& rawUniformBuffer, VkDeviceSize sizeofUniformData, const char* debugName = nullptr) {
	// data may need some padding if data not always in vec4 (4x float) format
	createPJBuffer(
		rawUniformBuffer,
		sizeofUniformData,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		debugName
	);
}

// ################################################################################################################################################################## //

/* Sets render targets for pje's Depth and MSAA*/
void createDepthAndMSAA(VkExtent3D imageSize) {
	/* MSAA IMAGE */
	VkImageCreateInfo msaaImageInfo;
	msaaImageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	msaaImageInfo.pNext = nullptr;
	msaaImageInfo.flags = 0;
	msaaImageInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	msaaImageInfo.format = pje::context.outputFormat;
	msaaImageInfo.extent = imageSize;
	msaaImageInfo.mipLevels = 1;
	msaaImageInfo.arrayLayers = 1;											// for Cubemaps :)
	msaaImageInfo.samples = pje::context.msaaFactor;
	msaaImageInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	msaaImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	msaaImageInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	msaaImageInfo.queueFamilyIndexCount = 0;
	msaaImageInfo.pQueueFamilyIndices = nullptr;
	msaaImageInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

	pje::context.msaaImage = make_unique<VkImage>();
	vkCreateImage(pje::context.logicalDevice, &msaaImageInfo, nullptr, pje::context.msaaImage.get());

	VkMemoryRequirements memReq;
	VkMemoryAllocateInfo memAllocInfo;

	/* Considers VK_SAMPLE_COUNT_4_BIT for memReq.size */
	vkGetImageMemoryRequirements(pje::context.logicalDevice, *pje::context.msaaImage, &memReq);

	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = memReq.size;
	memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	pje::context.result = vkAllocateMemory(pje::context.logicalDevice, &memAllocInfo, nullptr, &pje::context.msaaImageMemory);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createDepthRenderTarget::vkAllocateMemory" << endl;
		throw runtime_error("Error at createDepthRenderTarget::vkAllocateMemory");
	}
	vkBindImageMemory(pje::context.logicalDevice, *pje::context.msaaImage, pje::context.msaaImageMemory, 0);

	VkImageViewCreateInfo msaaImageViewInfo;
	msaaImageViewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	msaaImageViewInfo.pNext = nullptr;
	msaaImageViewInfo.flags = 0;
	msaaImageViewInfo.image = *pje::context.msaaImage;
	msaaImageViewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	msaaImageViewInfo.format = pje::context.outputFormat;
	msaaImageViewInfo.components = VkComponentMapping{
		VK_COMPONENT_SWIZZLE_IDENTITY	// properties r,g,b,a stay as their identity 
	};
	msaaImageViewInfo.subresourceRange = VkImageSubresourceRange{
		VK_IMAGE_ASPECT_COLOR_BIT,		// let driver know what this is used for
		0,								// for mipmap
		1,								// -||-
		0,								// for certain layer in VkImage
		1								// -||-
	};

	pje::context.msaaImageView = make_unique<VkImageView>();
	pje::context.result = vkCreateImageView(pje::context.logicalDevice, &msaaImageViewInfo, nullptr, pje::context.msaaImageView.get());
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createRenderTarget::vkCreateImageView" << endl;
		throw runtime_error("Error at createRenderTarget::vkCreateImageView");
	}

	/* DEPTH IMAGE */
	VkImageCreateInfo depthImageInfo;
	depthImageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.pNext = nullptr;
	depthImageInfo.flags = 0;
	depthImageInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	depthImageInfo.format = pje::context.depthFormat;						// TODO (civ)
	depthImageInfo.extent = imageSize;
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;											// for Cubemaps :)
	depthImageInfo.samples = pje::context.msaaFactor;
	depthImageInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	depthImageInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	depthImageInfo.queueFamilyIndexCount = 0;
	depthImageInfo.pQueueFamilyIndices = nullptr;
	depthImageInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

	pje::context.depthImage = make_unique<VkImage>();
	vkCreateImage(pje::context.logicalDevice, &depthImageInfo, nullptr, pje::context.depthImage.get());

	/* creating VkMemory without vmaFunctions - TEMPORARY */
	vkGetImageMemoryRequirements(pje::context.logicalDevice, *pje::context.depthImage, &memReq);

	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = memReq.size;
	memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	pje::context.result = vkAllocateMemory(pje::context.logicalDevice, &memAllocInfo, nullptr, &pje::context.depthImageMemory);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createRenderTarget::vkAllocateMemory" << endl;
		throw runtime_error("Error at createRenderTarget::vkAllocateMemory");
	}
	vkBindImageMemory(pje::context.logicalDevice, *pje::context.depthImage, pje::context.depthImageMemory, 0);

	VkImageViewCreateInfo depthImageViewInfo;
	depthImageViewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthImageViewInfo.pNext = nullptr;
	depthImageViewInfo.flags = 0;
	depthImageViewInfo.image = *pje::context.depthImage;
	depthImageViewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewInfo.format = pje::context.depthFormat;
	depthImageViewInfo.components = VkComponentMapping{
		VK_COMPONENT_SWIZZLE_IDENTITY	// properties r,g,b,a stay as their identity 
	};
	depthImageViewInfo.subresourceRange = VkImageSubresourceRange{
		VK_IMAGE_ASPECT_DEPTH_BIT,		// let driver know what this is used for
		0,								// for mipmap
		1,								// -||-
		0,								// for certain layer in VkImage
		1								// -||-
	};

	pje::context.depthImageView = make_unique<VkImageView>();
	pje::context.result = vkCreateImageView(pje::context.logicalDevice, &depthImageViewInfo, nullptr, pje::context.depthImageView.get());
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createRenderTarget::vkCreateImageView" << endl;
		throw runtime_error("Error at createRenderTarget::vkCreateImageView");
	}
}

// ################################################################################################################################################################## //

/* Cleanup of VkHandles on reset or on app termination */
void pje::cleanupRealtimeRendering(bool reset) {
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		vkDestroyFramebuffer(pje::context.logicalDevice, pje::context.swapchainFramebuffers[i], nullptr);
		vkDestroyImageView(pje::context.logicalDevice, pje::context.swapchainImageViews[i], nullptr);		// vkDestroyImage for swapchainImages NOT NECESSARY - handled by vkDestroySwapchainKHR
	}

	/* I definitely need vma.. */
	vkDestroyImageView(pje::context.logicalDevice, *pje::context.depthImageView, nullptr);
	vkFreeMemory(pje::context.logicalDevice, pje::context.depthImageMemory, nullptr);
	vkDestroyImage(pje::context.logicalDevice, *pje::context.depthImage, nullptr);
	vkDestroyImageView(pje::context.logicalDevice, *pje::context.msaaImageView, nullptr);
	vkFreeMemory(pje::context.logicalDevice, pje::context.msaaImageMemory, nullptr);
	vkDestroyImage(pje::context.logicalDevice, *pje::context.msaaImage, nullptr);

	/* should ONLY be used by stopVulkan() */
	if (!reset) {
		vkDestroyDescriptorPool(pje::context.logicalDevice, pje::context.descriptorPool, nullptr);			// also does vkFreeDescriptorSets() automatically

		vkFreeMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::stagingBuffer.buffer, nullptr);

		vkFreeMemory(pje::context.logicalDevice, pje::storeBoneRefs.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::storeBoneRefs.buffer, nullptr);
		vkFreeMemory(pje::context.logicalDevice, pje::storeBoneMatrices.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::storeBoneMatrices.buffer, nullptr);

		vkFreeMemory(pje::context.logicalDevice, pje::uniformsBuffer.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::uniformsBuffer.buffer, nullptr);
		vkFreeMemory(pje::context.logicalDevice, pje::indexBuffer.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::indexBuffer.buffer, nullptr);
		vkFreeMemory(pje::context.logicalDevice, pje::vertexBuffer.deviceMemory, nullptr);
		vkDestroyBuffer(pje::context.logicalDevice, pje::vertexBuffer.buffer, nullptr);

		vkDestroyPipeline(pje::context.logicalDevice, pje::context.pipeline, nullptr);
		vkDestroyDescriptorSetLayout(pje::context.logicalDevice, pje::context.descriptorSetLayout, nullptr);
		vkDestroyRenderPass(pje::context.logicalDevice, pje::context.renderPass, nullptr);
		vkDestroyPipelineLayout(pje::context.logicalDevice, pje::context.pipelineLayout, nullptr);

		vkDestroyCommandPool(pje::context.logicalDevice, pje::context.commandPool, nullptr);				// also does vkFreeCommandBuffers() automatically
		vkDestroySwapchainKHR(pje::context.logicalDevice, pje::context.swapchain, nullptr);
	}
}

/* Recording of context.commandBuffers - Declares what has to be rendered later */
void recordContextCommandBuffers(pje::PJMesh& renderable, uint32_t numberOfCommandBuffer) {
	// Defines CommandBuffer behavior
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;	// enables sending to GPU before GPU executes same command buffer
	commandBufferBeginInfo.pInheritanceInfo = nullptr;								// for secondary command buffer (see VkCommandBufferAllocateInfo.level)

	array<VkClearValue, 3> clearValues{};
	clearValues[0].color = VkClearColorValue{ 
		pje::context.clearValueDefault[0], 
		pje::context.clearValueDefault[1], 
		pje::context.clearValueDefault[2],
		pje::context.clearValueDefault[3]
	};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[2].color = VkClearColorValue{
		pje::context.clearValueDefault[0],
		pje::context.clearValueDefault[1],
		pje::context.clearValueDefault[2],
		pje::context.clearValueDefault[3]
	};

	// Telling command buffer which render pass to start
	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = pje::context.renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { pje::context.windowWidth, pje::context.windowHeight };
	renderPassBeginInfo.clearValueCount = 3;
	renderPassBeginInfo.pClearValues = clearValues.data();

	/* RECORDING of all context.commandBuffers */
	for (uint32_t i = 0; i < numberOfCommandBuffer; i++) {
		/* START RECORDING */
		pje::context.result = vkBeginCommandBuffer(pje::context.commandBuffers[i], &commandBufferBeginInfo);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkBeginCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[i] << endl;
			throw runtime_error("Error at vkBeginCommandBuffer");
		}

		renderPassBeginInfo.framebuffer = pje::context.swapchainFramebuffers[i];									// framebuffer that the current command buffer is associated with
		vkCmdBeginRenderPass(pje::context.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);		// connects CommandBuffer to Framebuffer ; CommandBuffer knows RenderPass

		// BIND => decide for an pipeline and use it for graphical calculation
		vkCmdBindPipeline(pje::context.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pje::context.pipeline);

		// SET dynamic states for vkCmdDraw
		VkViewport viewport{ 0.0f, 0.0f, pje::context.windowWidth, pje::context.windowHeight, 0.0f, 1.0f };
		vkCmdSetViewport(pje::context.commandBuffers[i], 0, 1, &viewport);
		VkRect2D scissor{ {0, 0}, {pje::context.windowWidth, pje::context.windowHeight} };
		vkCmdSetScissor(pje::context.commandBuffers[i], 0, 1, &scissor);

		// BIND => ordering to use certain buffer in VRAM
		array<VkDeviceSize, 1> offsets{ 0 };
		vkCmdBindDescriptorSets(
			pje::context.commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pje::context.pipelineLayout,
			0,
			1,
			&pje::context.descriptorSet,
			0,
			nullptr
		);

		// TODO ===> FOR LOOP - START

		vkCmdBindVertexBuffers(pje::context.commandBuffers[i], 0, 1, &pje::vertexBuffer.buffer, offsets.data());
		vkCmdBindIndexBuffer(pje::context.commandBuffers[i], pje::indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// DRAW => drawing on swapchain images
		vkCmdDrawIndexed(pje::context.commandBuffers[i], renderable.m_indices.size(), 1, 0, 0, 0);

		// TODO ===> FOR LOOP - START

		vkCmdEndRenderPass(pje::context.commandBuffers[i]);

		pje::context.result = vkEndCommandBuffer(pje::context.commandBuffers[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkEndCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[i] << endl;
			throw runtime_error("Error at vkEndCommandBuffer");
		}
		/* END RECORDING */
	}
}

/* Setup for Swapchain, Renderpass, Pipeline and CommandBufferPool */
void setupRealtimeRendering(bool reset = false) {
	/* swapchainInfo for building a Swapchain */
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.flags = 0;
	swapchainInfo.surface = pje::context.surface;
	swapchainInfo.minImageCount = pje::context.neededSurfaceImages;					// requires AT LEAST 2 (double buffering)
	swapchainInfo.imageFormat = pje::context.outputFormat;							// TODO ( choose dynamically )
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;				// TODO ( choose dynamically )
	swapchainInfo.imageExtent = VkExtent2D{ pje::context.windowWidth, pje::context.windowHeight };
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					// TODO ( an additional flag might be necessary )
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;						// exclusiv to single queue family at a time
	swapchainInfo.queueFamilyIndexCount = 0;										// enables access across multiple queue families => .imageSharingMode = VK_SHARING_MODE_CONCURRENT
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;				// no additional transformations
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// no see through images
	swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;							// VK_PRESENT_MODE_FIFO_KHR => VSYNC | VK_PRESENT_MODE_IMMEDIATE_KHR => UNLIMITED FPS
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = pje::context.swapchain;							// resizing image needs new swapchain

	/* Setting Swapchain with swapchainInfo to logicalDevice */
	pje::context.result = vkCreateSwapchainKHR(pje::context.logicalDevice, &swapchainInfo, nullptr, &pje::context.swapchain);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateSwapchainKHR" << endl;
		throw runtime_error("Error at vkCreateSwapchainKHR");
	}

	/* Gets images of Swapchain => should be already declared via swapchainInfo.minImageCount */
	pje::context.numberOfImagesInSwapchain = 0;
	vkGetSwapchainImagesKHR(pje::context.logicalDevice, pje::context.swapchain, &pje::context.numberOfImagesInSwapchain, nullptr);

	/* swapchainImages will hold the reference to VkImage(s), BUT to access them there has to be VkImageView(s) */
	auto swapchainImages = vector<VkImage>(pje::context.numberOfImagesInSwapchain);
	pje::context.result = vkGetSwapchainImagesKHR(pje::context.logicalDevice, pje::context.swapchain, &pje::context.numberOfImagesInSwapchain, swapchainImages.data());
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkGetSwapchainImagesKHR" << endl;
		throw runtime_error("Error at vkGetSwapchainImagesKHR");
	}
	
	/* ImageViewInfo for building ImageView ! TODO ! */
	VkImageViewCreateInfo imageViewInfo;
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = pje::context.outputFormat;			// TODO ( choose dynamically )
	imageViewInfo.components = VkComponentMapping{
		VK_COMPONENT_SWIZZLE_IDENTITY	// properties r,g,b,a stay as their identity 
	};
	imageViewInfo.subresourceRange = VkImageSubresourceRange{
		VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask : describes what kind of data is stored in image
		0,								// baseMipLevel
		1,								// levelCount		: describes how many MipMap levels exist
		0,								// baseArrayLayer	: for cubemaps
		1								// layerCount		: for cubemaps
	};

	/* ImageView gives access to images in swapchainImages */
	pje::context.swapchainImageViews = make_unique<VkImageView[]>(pje::context.numberOfImagesInSwapchain);

	/* Creates VkImageView for each VkImage in swapchain*/
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		imageViewInfo.image = swapchainImages[i];

		pje::context.result = vkCreateImageView(pje::context.logicalDevice, &imageViewInfo, nullptr, &pje::context.swapchainImageViews[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateImageView" << endl;
			throw runtime_error("Error at vkCreateImageView");
		}
	}

	createDepthAndMSAA(VkExtent3D{ pje::context.windowWidth, pje::context.windowHeight, 1 });

	/* VkCommandPool holds CommandBuffer which are necessary to tell Vulkan what to do */
	if (!reset) {
		VkCommandPoolCreateInfo commandPoolInfo;
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.pNext = nullptr;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;	// reset record of single frame buffer | set to buffer to 'transient' for optimisation
		commandPoolInfo.queueFamilyIndex = pje::context.choosenQueueFamily;			// must be the same as in VkDeviceQueueCreateInfo of the logical device & Queue Family 'VK_QUEUE_GRAPHICS_BIT' must be 1

		pje::context.result = vkCreateCommandPool(pje::context.logicalDevice, &commandPoolInfo, nullptr, &pje::context.commandPool);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateCommandPool" << endl;
			throw runtime_error("Error at vkCreateCommandPool");
		}
	}
	if (!reset) {
		/* RENDERPASS - START */

		VkAttachmentDescription attachmentDescriptionMSAA;
		attachmentDescriptionMSAA.flags = 0;
		attachmentDescriptionMSAA.format = pje::context.outputFormat;
		attachmentDescriptionMSAA.samples = pje::context.msaaFactor;
		attachmentDescriptionMSAA.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptionMSAA.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptionMSAA.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptionMSAA.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptionMSAA.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptionMSAA.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachmentDescriptionDepth;
		attachmentDescriptionDepth.flags = 0;
		attachmentDescriptionDepth.format = pje::context.depthFormat;
		attachmentDescriptionDepth.samples = pje::context.msaaFactor;
		attachmentDescriptionDepth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptionDepth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptionDepth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptionDepth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptionDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptionDepth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// VkAttachmentDescription is held by renderPass for its subpasses (VkAttachmentReference.attachment uses VkAttachmentDescription)
		VkAttachmentDescription attachmentDescriptionResolve;
		attachmentDescriptionResolve.flags = 0;
		attachmentDescriptionResolve.format = pje::context.outputFormat;					// has the same format as the swapchainInfo.imageFormat
		attachmentDescriptionResolve.samples = VK_SAMPLE_COUNT_1_BIT;						// FlagBit => multisampling n_BIT
		attachmentDescriptionResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// what happens with the buffer after loading data
		attachmentDescriptionResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// what happens with the buffer after storing data
		attachmentDescriptionResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
		attachmentDescriptionResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
		attachmentDescriptionResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// buffer as input
		attachmentDescriptionResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;			// buffer as output

		VkAttachmentReference attachmentRefMSAA;
		attachmentRefMSAA.attachment = 0;
		attachmentRefMSAA.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference attachmentRefDepth;
		attachmentRefDepth.attachment = 1;
		attachmentRefDepth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference attachmentRefResolve;
		attachmentRefResolve.attachment = 2;										// index of attachment description
		attachmentRefResolve.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// CONVERTING: desc.initialLayout -> ref.layout -> desc.finalLayout

		// each render pass holds 1+ sub render passes | equals one run through pipeline
		VkSubpassDescription subpassDescription;
		subpassDescription.flags = 0;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// what kind of computation will be done
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;							// actual vertices that being used for shader computation
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentRefMSAA;				// which VkAttachmentDescription (attribute bundle) of Renderpass will be used
		subpassDescription.pResolveAttachments = &attachmentRefResolve;			// -||-
		subpassDescription.pDepthStencilAttachment = &attachmentRefDepth;		// -||-
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;

		// VkSubpassDependency can be seen as a semaphore between subpasses for their micro tasks
		VkSubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;									// waiting for the next finishing subpass
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// src scope has to finish that state ..
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// .. before dst is allowed to start this state
		subpassDependency.srcAccessMask = VK_ACCESS_NONE;
		subpassDependency.dstAccessMask = VK_ACCESS_NONE;
		subpassDependency.dependencyFlags = 0;

		VkSubpassDependency subpassDependencyDepth;
		subpassDependencyDepth.srcSubpass = VK_SUBPASS_EXTERNAL;							// doesn't render on rendertarget before finishing previous subpass
		subpassDependencyDepth.dstSubpass = 0;
		subpassDependencyDepth.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencyDepth.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencyDepth.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencyDepth.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		subpassDependencyDepth.dependencyFlags = 0;

		array<VkAttachmentDescription, 3> attachmentDescriptions = {
			attachmentDescriptionMSAA, attachmentDescriptionDepth, attachmentDescriptionResolve
		};

		array<VkSubpassDependency, 2> subpassDependencies = {
			subpassDependency, subpassDependencyDepth
		};

		/* renderPass represents a single execution of the rendering pipeline */
		VkRenderPassCreateInfo renderPassInfo;
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.flags = 0;
		renderPassInfo.attachmentCount = 3;
		renderPassInfo.pAttachments = attachmentDescriptions.data();			// for subpassDescription's VkAttachmentReference(s)
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;						// all subpasses of the current render pass
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = subpassDependencies.data();

		/* Creating RenderPass for VkGraphicsPipelineCreateInfo */
		pje::context.result = vkCreateRenderPass(pje::context.logicalDevice, &renderPassInfo, nullptr, &pje::context.renderPass);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateRenderPass" << endl;
			throw runtime_error("Error at vkCreateRenderPass");
		}

		/* RENDERPASS - END */

		/* PIPELINE BUILDING - START */

		/* Viewport */
		VkViewport initViewport;
		initViewport.x = 0.0f;							// upper left corner
		initViewport.y = 0.0f;							// upper left corner
		initViewport.width = pje::context.windowWidth;
		initViewport.height = pje::context.windowHeight;
		initViewport.minDepth = 0.0f;
		initViewport.maxDepth = 1.0f;

		/* Scissor */
		VkRect2D initScissor;							// defines what part of the viewport will be used
		initScissor.offset = { 0, 0 };
		initScissor.extent = { pje::context.windowWidth,  pje::context.windowHeight };

		/* viewportInfo => combines viewport and scissor for the pipeline */
		VkPipelineViewportStateCreateInfo viewportInfo;
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.pNext = nullptr;
		viewportInfo.flags = 0;
		viewportInfo.viewportCount = 1;							// number of viewport
		viewportInfo.pViewports = &initViewport;
		viewportInfo.scissorCount = 1;							// number of scissors
		viewportInfo.pScissors = &initScissor;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.pNext = nullptr;
		inputAssemblyInfo.flags = 0;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// how single vertices will be assembled
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		auto vertexBindingDesc = pje::PJVertex::getInputBindingDesc();
		auto vertexAttribsDesc = pje::PJVertex::getInputAttributeDesc();

		/* Usally sizes of vertices and their attributes are defined here
		 * It has similar tasks to OpenGL's glBufferData(), glVertexAttribPointer() etc. (Vertex Shader => layout in) */
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.pNext = nullptr;
		vertexInputInfo.flags = 0;
		vertexInputInfo.vertexBindingDescriptionCount = 1;							// number of pVertexBindingDescriptionS
		vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;			// binding index for AttributeDescription | stride for sizeof(Vertex) | inputRate to decide between per vertex or per instance
		vertexInputInfo.vertexAttributeDescriptionCount = vertexAttribsDesc.size();	// number of pVertexAttribute
		vertexInputInfo.pVertexAttributeDescriptions = vertexAttribsDesc.data();	// location in shader code | binding is binding index of BindingDescription | format defines vertex datatype | offset defines start of the element

		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.pNext = nullptr;
		rasterizationInfo.flags = 0;
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;						// if true primitives won't be rendered but they may be used for other calculations
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_NONE;			// backface culling
		rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.depthBiasConstantFactor = 0.0f;
		rasterizationInfo.depthBiasClamp = 0.0f;
		rasterizationInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationInfo.lineWidth = 1.0f;

		// Blending on one image of swapchain (for one framebuffer)
		//	[Pseudocode]
		//	if (blendEnable)
		//		color.rgb = (srcColorBlendFactor * newColor.rgb) [colorBlendOp] (dstColorBlendFactor * oldColor.rgb)
		//		color.a	  = (srcAlphaBlendFactor * newColor.a)   [alphaBlendOp] (dstAlphaBlendFactor * oldColor.a)
		//	else
		//		color = newColor
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.pNext = nullptr;
		colorBlendInfo.flags = 0;
		colorBlendInfo.logicOpEnable = VK_FALSE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_NO_OP;
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachment;
		fill_n(colorBlendInfo.blendConstants, 4, 0.0f);			// sets rgba to 0.0f

		/* Declaring VkDescriptorSetLayout for Pipeline */
		createDescriptorSetLayout(pje::context.descriptorSetLayout);

		// pipelineLayoutInfo declares LAYOUTS and UNIFORMS for compiled shader code
		VkPipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		pipelineLayoutInfo.flags = 0;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &pje::context.descriptorSetLayout;		//  layout(set = n..)
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		pje::context.result = vkCreatePipelineLayout(pje::context.logicalDevice, &pipelineLayoutInfo, nullptr, &pje::context.pipelineLayout);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreatePipelineLayout" << endl;
			throw runtime_error("Error at vkCreatePipelineLayout");
		}

		// Multisampling for Antialiasing
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.pNext = nullptr;
		multisampleInfo.flags = 0;
		multisampleInfo.rasterizationSamples = pje::context.msaaFactor;		// precision of the sampling
		multisampleInfo.sampleShadingEnable = VK_FALSE;						// enables super sampling
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
		depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateInfo.pNext = nullptr;
		depthStencilStateInfo.flags = 0;
		depthStencilStateInfo.depthTestEnable = VK_TRUE;
		depthStencilStateInfo.depthWriteEnable = VK_TRUE;
		depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilStateInfo.minDepthBounds = 0.0f;
		depthStencilStateInfo.maxDepthBounds = 1.0f;

		/* Defining dynamic members for VkGraphicsPipelineCreateInfo */
		array<VkDynamicState, 2> dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pNext = nullptr;
		dynamicStateInfo.flags = 0;
		dynamicStateInfo.dynamicStateCount = dynamicStates.size();
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		/* CreateInfo for the actual pipeline */
		VkGraphicsPipelineCreateInfo pipelineInfo;
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.flags = 0;
		pipelineInfo.stageCount = 2;									// number of programmable stages
		pipelineInfo.pStages = pje::context.shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;				// input for vertex shader (start of pipeline)
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pTessellationState = nullptr;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &rasterizationInfo;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pDepthStencilState = &depthStencilStateInfo;		// depth buffer
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;					// parts being changeable without building new pipeline
		pipelineInfo.layout = pje::context.pipelineLayout;
		pipelineInfo.renderPass = pje::context.renderPass;				// holds n subpasses and subpass attachments
		pipelineInfo.subpass = 0;										// index of .subpass
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;				// deriving from other pipeline to reduce loading time
		pipelineInfo.basePipelineIndex = -1;							// good coding style => invalid index

		/* Pipeline Creation */
		pje::context.result = vkCreateGraphicsPipelines(pje::context.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pje::context.pipeline);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateGraphicsPipelines" << endl;
			throw runtime_error("Error at vkCreateGraphicsPipelines");
		}
	}
	/* PIPELINE BUILDING - END */

	if (!reset) {
		/* Allocation of VkBuffer to transfer CPU vertices onto GPU */
		sendDataToVRAM(pje::vertexBuffer, pje::loadedModels[0].meshes[0].m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "vertexBuffer");
		sendDataToVRAM(pje::indexBuffer, pje::loadedModels[0].meshes[0].m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "indexBuffer");
		
		/* TEMPORARY */
		createPJBuffer(pje::storeBoneRefs, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "storeBoneRefs");
		createPJBuffer(pje::storeBoneMatrices, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "storeBoneMatrices");
	}

	/* uniforms/storage buffer for shaders in pipeline */
	if (!reset) {
		/* Allocation of COHERENT VkBuffer to transfer uniforms onto GPU */
		createUniformBuffer(pje::uniformsBuffer, sizeof(pje::Uniforms), "uniformsBuffer");
		/* Allocation of VkDescriptorPool und VkDescriptorSet to update uniforms */
		createDescriptorPool(pje::context.descriptorPool);
		createDescriptorSet(pje::context.descriptorSet);
		/* Allocation of Uniforms and Storage Buffers to DescriptorSet*/
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pje::uniformsBuffer);
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pje::storeBoneRefs);
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pje::storeBoneMatrices);
	}

	/* Framebuffer => bridge buffer between CommandBuffer and Rendertargets/Image Views (Vulkan communication buffer) */
	pje::context.swapchainFramebuffers = make_unique<VkFramebuffer[]>(pje::context.numberOfImagesInSwapchain);
	VkFramebufferCreateInfo framebufferInfo;
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;
	framebufferInfo.flags = 0;
	framebufferInfo.renderPass = pje::context.renderPass;
	framebufferInfo.attachmentCount = 3;										// one framebuffer may store multiple imageViews
	framebufferInfo.width = pje::context.windowWidth;							// dimension
	framebufferInfo.height = pje::context.windowHeight;							// dimension
	framebufferInfo.layers = 1;													// dimension

	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		array<VkImageView, 3> rendertargets{
			*pje::context.msaaImageView,
			*pje::context.depthImageView,
			pje::context.swapchainImageViews[i]
		};

		// VkAttachmentDescription of VkRenderPass describes rendertargets
		framebufferInfo.pAttachments = rendertargets.data();	// imageView => reference to image in swapchain

		pje::context.result = vkCreateFramebuffer(pje::context.logicalDevice, &framebufferInfo, nullptr, &pje::context.swapchainFramebuffers[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateFramebuffer" << endl;
			throw runtime_error("Error at vkCreateFramebuffer");
		}
	}
	
	/* CommandBuffer for rendering */
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = pje::context.commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// primary => processed directly | secondary => invoked by primary
	commandBufferAllocateInfo.commandBufferCount = pje::context.numberOfImagesInSwapchain;	// each buffer in swapchain needs/has a dedicated command buffer

	pje::context.commandBuffers = make_unique<VkCommandBuffer[]>(pje::context.numberOfImagesInSwapchain);
	pje::context.result = vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, pje::context.commandBuffers.get());
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at setupRealtimeRendering::vkAllocateCommandBuffers" << endl;
		throw runtime_error("Error at setupRealtimeRendering::vkAllocateCommandBuffers");
	}
	
	/* Record CommandBuffers designated to rendering */
	recordContextCommandBuffers(pje::loadedModels[0].meshes[0], pje::context.numberOfImagesInSwapchain);
}

/* Callback for glfwSetWindowSizeCallback */
void resetRealtimeRendering(GLFWwindow* window, int width, int height) {
	if (width <= 0 || height <= 0) {
		pje::context.isWindowMinimized = true;
		return;
	}
	else if (pje::context.isWindowMinimized) {
		pje::context.isWindowMinimized = false;
	}

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		pje::context.physicalDevices[pje::context.choosenPhysicalDevice],
		pje::context.surface,
		&capabilities
	);
	if (width > capabilities.maxImageExtent.width)
		width = capabilities.maxImageExtent.width;
	if (height > capabilities.maxImageExtent.height)
		height = capabilities.maxImageExtent.height;

	pje::context.windowWidth = width;
	pje::context.windowHeight = height;
	VkSwapchainKHR oldSwapchain = pje::context.swapchain;

	vkDeviceWaitIdle(pje::context.logicalDevice);

	pje::cleanupRealtimeRendering(true);
	setupRealtimeRendering(true);
	vkDestroySwapchainKHR(pje::context.logicalDevice, oldSwapchain, nullptr);
}

// ################################################################################################################################################################## //

/* Sets callback functions */
void setGlfwCallbacks() {
	/* Sets callback function on resize */
	glfwSetWindowSizeCallback(pje::context.window, resetRealtimeRendering);
}

/* Creates a window |  */
int pje::startGlfw3(const char* windowName) {
	glfwInit();

	if (glfwVulkanSupported() == GLFW_FALSE) {
		cout << "Error at glfwVulkanSupported" << endl;
		return -1;
	}

	/* Settings for upcoming window creation */
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	pje::context.window = glfwCreateWindow(pje::context.windowWidth, pje::context.windowHeight, windowName, nullptr, nullptr);
	if (pje::context.window == NULL) {
		cout << "Error at glfwCreateWindow" << endl;
		glfwTerminate();
		return -1;
	}

	setGlfwCallbacks();

	return 0;
}

/* Closes created window */
void pje::stopGlfw3() {
	glfwDestroyWindow(pje::context.window);
	glfwTerminate();

	cout << "[DEBUG] GLFW ressources cleaned up." << endl;
}

// ################################################################################################################################################################## //

/* Sets pje::context.choosenPhysicalDevice based on needed attributes */
void selectGPU(const vector<VkPhysicalDevice> physicalDevices, VkPhysicalDeviceType preferredType, const vector<VkQueueFlagBits> neededFamilyQueueAttributes, uint32_t neededSurfaceImages, VkFormat imageColorFormat, VkPresentModeKHR neededPresentationMode) {
	if (physicalDevices.size() == 0)
		throw runtime_error("[ERROR] No GPU was found on this computer.");
	
	const uint8_t AMOUNT_OF_REQUIREMENTS = 5;

	for (const auto& physicalDevice : physicalDevices) {
		uint8_t requirementCounter = 0;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		cout << "\n[OS] GPU [" << props.deviceName << "] was found." << endl;
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			cout << "[OS] GPU is VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU." << endl;
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			cout << "[OS] GPU is VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU." << endl;

		if (props.deviceType == preferredType) {
			++requirementCounter;
			cout << "[OS] Preferred device type was found." << endl;
		}
		else {
			cout << "\t[OS] Preferred device type wasn't found." << endl;
			break;
		}

		{
			uint8_t counter(0);
			bool foundQueueFamily = false;

			uint32_t numberOfQueueFamilies = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, nullptr);

			auto queueFamilyProps = std::vector<VkQueueFamilyProperties>(numberOfQueueFamilies);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, queueFamilyProps.data());

			for (uint32_t i = 0; i < numberOfQueueFamilies; i++) {					// inspecting each queue family of that device
				for (const auto& attribute : neededFamilyQueueAttributes) {			// one queue family has to fulfill all attributes on its own
					if ((attribute & queueFamilyProps[i].queueFlags) != 0)			// check : bitwise AND
						++counter;
				}
				if (counter == neededFamilyQueueAttributes.size()) {				// check : does the current family queue satisfy all requirements?
					foundQueueFamily = true;
					pje::context.choosenQueueFamily = i;
					break;
				}
				else {
					counter = 0;
				}
			}

			if (!foundQueueFamily) {
				cout << "\t[OS] No suitable queue family was found." << endl;
				break;
			}
			else {
				++requirementCounter;
				cout << "[OS] pje::context.choosenQueueFamily was set to : " << pje::context.choosenQueueFamily << endl;
			}
		}

		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pje::context.surface, &surfaceCapabilities);

			if (surfaceCapabilities.minImageCount >= neededSurfaceImages) {
				++requirementCounter;
				cout << "[OS] Physical device supports needed amount of surface images" << endl;
			}
			else {
				cout << "\t[OS] Physical device cannot provide the needed amount of surface images" << endl;
				break;
			}
		}

		{
			uint32_t numberOfFormats = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pje::context.surface, &numberOfFormats, nullptr);
			auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(numberOfFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pje::context.surface, &numberOfFormats, surfaceFormats.data());

			bool foundVkFormat = false;

			for (uint32_t i = 0; i < numberOfFormats; i++) {
				if (surfaceFormats[i].format == imageColorFormat) {
					foundVkFormat = true;
					break;
				}
			}

			if (!foundVkFormat) {
				cout << "\t[OS] Required VkFormat wasn't provided by the physical device" << endl;
				break;
			}
			else {
				++requirementCounter;
				cout << "[OS] Physical device supports required VkFormat" << endl;
			}
		}

		{
			uint32_t numberOfPresentationModes = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pje::context.surface, &numberOfPresentationModes, nullptr);
			auto presentModes = std::vector<VkPresentModeKHR>(numberOfPresentationModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pje::context.surface, &numberOfPresentationModes, presentModes.data());

			bool foundPresentMode = false;

			for (uint32_t i = 0; i < numberOfPresentationModes; i++) {
				if (presentModes[i] == neededPresentationMode) {
					foundPresentMode = true;
					
					break;
				}
			}

			if (!foundPresentMode) {
				cout << "\t[OS] Required VkPresentModeKHR wasn't provided by the physical device" << endl;
				break;
			}
			else {
				++requirementCounter;
				cout << "[OS] Physical device supports required VkPresentModeKHR" << endl;
			}
		}

		if (requirementCounter == AMOUNT_OF_REQUIREMENTS) {
			cout << "[OS] PJEngine is choosing " << props.deviceName << ".\n" << endl;
			pje::context.choosenPhysicalDevice = 0;
			return;
		}
		else {
			cout << "\t[OS] PJEngine declined " << props.deviceName << ".\n" << endl;
		}
	}

	throw runtime_error("[ERROR] No suitable GPU was found for PJEngine.");
}

/* Setup of pje::context */
int pje::startVulkan() {
	/* appInfo used for instanceInfo */
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;				// sType for (GPU) driver
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = pje::context.appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PJ Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	cout << "Engine started.." << endl;

	/* LAYERs act like hooks to intercept Vulkan API calls */
	uint32_t numberOfInstanceLayers = 0;
	vkEnumerateInstanceLayerProperties(&numberOfInstanceLayers, nullptr);

	auto availableInstanceLayers = vector<VkLayerProperties>(numberOfInstanceLayers);
	vkEnumerateInstanceLayerProperties(&numberOfInstanceLayers, availableInstanceLayers.data());

	cout << "\n[OS] Available Instance Layers:\t" << numberOfInstanceLayers << endl;
	for (uint32_t i = 0; i < numberOfInstanceLayers; i++) {
		cout << "\tLayer:\t" << availableInstanceLayers[i].layerName << endl;
		cout << "\t\t\t" << availableInstanceLayers[i].description << endl;
	}

	vector<const char*> usedInstanceLayers;
	for (const auto& each : availableInstanceLayers) {
		if (strcmp(each.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
			cout << "\n[DEBUG] Validation Layer for Instance Dispatch Chain found." << endl;
			usedInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
		}
	}
	if (usedInstanceLayers.empty()) {
		cout << "\n[DEBUG] Validation Layer for Instance Dispatch Chain NOT found." << endl;
	}

	/* EXTENSIONS deliver optional functionalities provided by layers, loader or ICD */
	uint32_t numberOfInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfInstanceExtensions, nullptr);

	auto availableInstanceExtensions = vector<VkExtensionProperties>(numberOfInstanceExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfInstanceExtensions, availableInstanceExtensions.data());

	cout << "\n[OS] Available Instance Extensions:\t" << numberOfInstanceExtensions << endl;
	for (uint32_t i = 0; i < numberOfInstanceExtensions; i++) {
		cout << "\tExtension:\t" << availableInstanceExtensions[i].extensionName << endl;
	}

	/* [GLFW] GET ALL GLFW INSTANCE EXTENSIONS for current OS */
	uint32_t numberOfRequiredGlfwExtensions = 0;
	vector<const char*> usedInstanceExtensions;

	{
		const char** requiredGlfwExtensions = glfwGetRequiredInstanceExtensions(&numberOfRequiredGlfwExtensions);
		uint8_t counter(0);

		cout << "\n[GLFW] REQUIRED Instance Extensions:\t" << numberOfRequiredGlfwExtensions << endl;

		for (uint32_t i = 0; i < numberOfRequiredGlfwExtensions; i++) {
			cout << "\tExtension:\t" << requiredGlfwExtensions[i] << endl;

			for (const auto& each : availableInstanceExtensions) {
				if (strcmp(each.extensionName, requiredGlfwExtensions[i]) == 0) {
					++counter;
					cout << "\t\t\tExtension found." << endl;
					usedInstanceExtensions.push_back(requiredGlfwExtensions[i]);
				}
			}
		}

		if (counter == numberOfRequiredGlfwExtensions) {
			cout << "\n[DEBUG] All required GLFW Extensions found." << endl;
		}
		else {
			cout << "\n[ERROR] Not all required GLFW Extensions found." << endl;
			return -1;
		}
	}

	/* Configure validation (extension) features */
#ifndef NDEBUG
	usedInstanceExtensions.push_back("VK_EXT_debug_utils");
	VkValidationFeaturesEXT validationFeatures{ VkStructureType::VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
#endif

	/* instanceInfo used for the actual Vulkan instance */
	/* current LAYERs and EXTENSIONs:					*/
	/* - Validationlayer && GLFW Extensions				*/
	VkInstanceCreateInfo instanceInfo;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
#ifdef NDEBUG
	instanceInfo.pNext = nullptr;
	instanceInfo.enabledLayerCount = 0;
	instanceInfo.ppEnabledLayerNames = nullptr;
#else
	instanceInfo.pNext = &validationFeatures;
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(usedInstanceLayers.size());
	instanceInfo.ppEnabledLayerNames = usedInstanceLayers.data();
#endif
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(usedInstanceExtensions.size());
	instanceInfo.ppEnabledExtensionNames = usedInstanceExtensions.data();

	/* Vulkan Loader loads layers */
	pje::context.result = vkCreateInstance(&instanceInfo, nullptr, &pje::context.vulkanInstance);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateInstance" << endl;
		return pje::context.result;
	}

	/* Assign function pointer to extension */
#ifndef NDEBUG
	vef::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(pje::context.vulkanInstance, "vkSetDebugUtilsObjectNameEXT"));
#endif

	/* VkSurfaceKHR => creates surface based on an existing GLFW Window */
	pje::context.result = glfwCreateWindowSurface(pje::context.vulkanInstance, pje::context.window, nullptr, &pje::context.surface);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at glfwCreateWindowSurface" << endl;
		return pje::context.result;
	}

	/* Vulkan differs between PHYSICAL and LOGICAL GPU reference */
	/* A physical reference is necessary for a logical reference */
	uint32_t numberOfPhysicalDevices = 0;

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter):
		1) get number of GPUs and stores value in 2nd parameter */
	vkEnumeratePhysicalDevices(pje::context.vulkanInstance, &numberOfPhysicalDevices, nullptr);

	pje::context.physicalDevices = vector<VkPhysicalDevice>(numberOfPhysicalDevices);

	/*	vkEnumeratePhysicalDevices has two functionalities:
		2) get reference for a number of GPUs and store them in an array of physical devices */
	vkEnumeratePhysicalDevices(pje::context.vulkanInstance, &numberOfPhysicalDevices, pje::context.physicalDevices.data());

	cout << "\n[OS] Number of GPUs:\t\t" << numberOfPhysicalDevices << endl;

	/* Shows some properties and features for each available GPU */
	for (uint32_t i = 0; i < numberOfPhysicalDevices; i++) {
		pje::debugPhysicalDeviceStats(pje::context.physicalDevices[i]);
	}

	/* ########## Decides on the right GPU and QueueFamily for PJEngine ########## */
	/* Sets pje::context.choosenPhysicalDevice and pje::context.choosenQueueFamily */
	selectGPU(
		pje::context.physicalDevices,
		pje::context.preferredPhysicalDeviceType,
		pje::context.neededFamilyQueueAttributes,
		pje::context.neededSurfaceImages,
		pje::context.outputFormat,
		pje::context.neededPresentationMode
	);

	array<float, 1> queuePriorities { 1.0f };

	/* deviceQueueCreateInfo used for deviceInfo => choose ammount of queue of a certain queue family */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext = nullptr;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = pje::context.choosenQueueFamily;		// TODO ( analyse available families before setting one )
	deviceQueueInfo.queueCount = 1;											// TODO ( analyse available queues in family first )
	deviceQueueInfo.pQueuePriorities = queuePriorities.data();

	/* EXTENSIONS on device level */
	auto deviceExtensions = array { "VK_KHR_swapchain" };					// Swapchains => Real Time Rendering
	
	/* all device features for Vulkan 1.1 and upwards */
	VkPhysicalDeviceFeatures2 coreDeviceFeature { VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

	/* deviceInfo declares what resources will be claimed */
	/* deviceInfo is necessary for a logical reference    */
	VkDeviceCreateInfo deviceInfo;
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &coreDeviceFeature;
	deviceInfo.flags = 0;
	deviceInfo.queueCreateInfoCount = 1;										// number of VkDeviceQueueCreateInfo (1+ possible)
	deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceInfo.pEnabledFeatures = nullptr;

	/* Checking whether Swapchains are usable or not on physical device */
	VkBool32 surfaceSupportsSwapchain = false;
	pje::context.result = vkGetPhysicalDeviceSurfaceSupportKHR(
		pje::context.physicalDevices[pje::context.choosenPhysicalDevice], 
		0, 
		pje::context.surface, 
		&surfaceSupportsSwapchain
	);

	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkGetPhysicalDeviceSurfaceSupportKHR" << endl;
		return pje::context.result;
	}
	if (!surfaceSupportsSwapchain) {
		cout << "Swapchain isn't supported by choosen GPU!" << endl;
		return surfaceSupportsSwapchain;
	}

	/* LOGICAL GPU reference */
	pje::context.result = vkCreateDevice(
		pje::context.physicalDevices[pje::context.choosenPhysicalDevice], 
		&deviceInfo, 
		nullptr, 
		&pje::context.logicalDevice
	);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateDevice" << endl;
		return pje::context.result;
	}

	/* Getting Queue of some logical device to assign commandBuffer later */
	vkGetDeviceQueue(pje::context.logicalDevice, pje::context.choosenQueueFamily, 0, &pje::context.queueForPrototyping);

	/* ############## LOADING SHADERS ############## */

	/* Reading compiled shader code into RAM */
	auto shaderBasicVert = pje::readSpirvFile("assets/shaders/basic.vert.spv");
	auto shaderBasicFrag = pje::readSpirvFile("assets/shaders/basic.frag.spv");
	cout << "\n[DEBUG] Basic Shader Bytes\n"
		"\tVertex Shader:\t" << shaderBasicVert.size() << "\n"
		"\tFragment Shader:" << shaderBasicFrag.size() << endl;

	/* Transforming shader code of type vector<char> into VkShaderModule */
	pje::createShaderModule(shaderBasicVert, pje::context.shaderModuleBasicVert);
	pje::createShaderModule(shaderBasicFrag, pje::context.shaderModuleBasicFrag);

	/* Ensures that pje::shaderStageInfos are empty before filling with data */
	if (!pje::context.shaderStageInfos.empty())
		clearShaderStages();

	/* Wraping Shader Modules in a distinct CreateInfo for the Shaderpipeline and adding them to engine's stageInfos */
	pje::addShaderModuleToShaderStages(pje::context.shaderModuleBasicVert, VK_SHADER_STAGE_VERTEX_BIT);
	pje::addShaderModuleToShaderStages(pje::context.shaderModuleBasicFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
	cout << "[DEBUG] Programmable Pipeline Stages\n"
		"\tpje::shaderStageInfos: " << pje::context.shaderStageInfos.size() << endl;

	/* ############## SETUP FOR REAL TIME RENDERING ############## */

	/* Creates unsignaled fence for pje::stagingBuffer */
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = 0;
	vkCreateFence(pje::context.logicalDevice, &fenceInfo, nullptr, &pje::context.fenceCopiedBuffer);

	setupRealtimeRendering();

	/* ############## SETUP SEMAPHORES AND FENCES ############## */

	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	pje::context.result = vkCreateSemaphore(pje::context.logicalDevice, &semaphoreInfo, nullptr, &pje::context.semaphoreSwapchainImageReceived);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateSemaphore" << endl;
		return pje::context.result;
	}
	pje::context.result = vkCreateSemaphore(pje::context.logicalDevice, &semaphoreInfo, nullptr, &pje::context.semaphoreRenderingFinished);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateSemaphore" << endl;
		return pje::context.result;
	}

	/* Creates signaled fence for image rendering */
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

	pje::context.result = vkCreateFence(pje::context.logicalDevice, &fenceInfo, nullptr, &pje::context.fenceRenderFinished);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateFence" << endl;
		return pje::context.result;
	}

	/* DEBUG for debugUtils */
#ifndef NDEBUG
	setObjectNames(pje::context);
#endif

	return 0;
}

/* Cleanup of pje::context */
void pje::stopVulkan() {
	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(pje::context.logicalDevice);

	/* vkDestroy bottom-up */
	vkDestroyFence(pje::context.logicalDevice, pje::context.fenceRenderFinished, nullptr);
	vkDestroyFence(pje::context.logicalDevice, pje::context.fenceCopiedBuffer, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreSwapchainImageReceived, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreRenderingFinished, nullptr);

	/* vkDestroy vkFree etc. */
	cleanupRealtimeRendering();

	vkDestroyShaderModule(pje::context.logicalDevice, pje::context.shaderModuleBasicVert, nullptr);
	vkDestroyShaderModule(pje::context.logicalDevice, pje::context.shaderModuleBasicFrag, nullptr);
	clearShaderStages();

	vkDestroyDevice(pje::context.logicalDevice, nullptr);
	vkDestroySurfaceKHR(pje::context.vulkanInstance, pje::context.surface, nullptr);
	vkDestroyInstance(pje::context.vulkanInstance, nullptr);

	cout << "[DEBUG] Vulkan API ressources cleaned up." << endl;
}

// ################################################################################################################################################################## //

/* Updates a [VK_MEMORY_PROPERTY_HOST_COHERENT_BIT]-VkBuffer */
void updateUniformsBuffer(VkDeviceMemory& deviceMemory) {
	/* Updating VkBuffer (uniform) on VRAM */
	void* rawPointer;
	vkMapMemory(pje::context.logicalDevice, deviceMemory, 0, VK_WHOLE_SIZE, 0, &rawPointer);
	memcpy(rawPointer, &pje::uniforms, sizeof(pje::uniforms));
	vkUnmapMemory(pje::context.logicalDevice, pje::uniformsBuffer.deviceMemory);
}

/* Modifying matrices in relation to passed time since app start */
void updateMatrices() {
	auto frameTimePoint = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(frameTimePoint - pje::context.startTimePoint).count() / 1000.0f;

	pje::uniforms.modelMatrix = glm::rotate(
		glm::mat4(1.0f),						// identity matrix
		duration * glm::radians(20.0f),
		glm::vec3(0.0f, -1.0f, 0.0f)
	);

	pje::uniforms.viewMatrix = glm::lookAt(		// inverse modelMatrix_cam
		glm::vec3(4.0f, 2.0f, 4.0f),			// eye in world space (usally world moves around camera and NOT the other way around)
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	pje::uniforms.projectionMatrix = glm::perspective(
		glm::radians(60.0f),
		pje::context.windowWidth / (float)pje::context.windowHeight,
		0.1f,
		10.0f
	);

	/* Vulkan y axis goes down | OpenGL y axis goes up */
	pje::uniforms.projectionMatrix[1][1] *= -1;

	pje::uniforms.mvp = pje::uniforms.projectionMatrix * pje::uniforms.viewMatrix * pje::uniforms.modelMatrix;

	updateUniformsBuffer(pje::uniformsBuffer.deviceMemory);
}

/* Creates render calls via CommandBuffer handled by VkFence and VkSemaphore */
/* drawFrameOnSurface has 3 steps
 * 1) get an image from swapchain
 * 2) render inside of selected image
 * 3) give back image to swapchain to present it
 * 
 *    VkFence => CPU semaphore | VkSemaphores => GPU semaphore
 */
void drawFrameOnSurface() {
	if (!pje::context.isWindowMinimized) {
		/* CPU waits here for signaled fence(s)*/
		pje::context.result = vkWaitForFences(
			pje::context.logicalDevice,
			1,
			&pje::context.fenceRenderFinished,
			VK_TRUE,
			numeric_limits<uint64_t>::max()
		);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkWaitForFences" << endl;
			return;
		}

		/* Unsignals fence(s) */
		pje::context.result = vkResetFences(
			pje::context.logicalDevice,
			1,
			&pje::context.fenceRenderFinished
		);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkResetFences" << endl;
			return;
		}

		uint32_t imageIndex;
		pje::context.result = vkAcquireNextImageKHR(		// async => GPU can already compute without having a rendertarget
			pje::context.logicalDevice,
			pje::context.swapchain,
			numeric_limits<uint64_t>::max(),				// timeout in ns before abort
			pje::context.semaphoreSwapchainImageReceived,	// semaphore	=> only visible on GPU side
			VK_NULL_HANDLE,									// fences		=> like semaphores ; usable inside of Cpp Code
			&imageIndex
		);
		if (pje::context.result != VK_SUCCESS) {			// VK_SUBOPTIMAL etc. should be handled here
			throw runtime_error("Could not acquire image from swapchain.");
		}

		array<VkPipelineStageFlags, 1> waitStageMask{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	// allows async rendering behavior
		};

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &pje::context.semaphoreSwapchainImageReceived;
		submitInfo.pWaitDstStageMask = waitStageMask.data();						// wait at this point in the pipeline until pWaitSemaphore is signaled
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &pje::context.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &pje::context.semaphoreRenderingFinished;	// which semaphore will be triggered when submit step done

		/* Submitting CommandBuffer to queue => ACTUAL RENDERING */
		/* Fence must be unsignaled to proceed and signals fence */
		pje::context.result = vkQueueSubmit(
			pje::context.queueForPrototyping,
			1,
			&submitInfo,
			pje::context.fenceRenderFinished
		);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkQueueSubmit" << endl;
			return;
		}

		/* VkSwapchainKHR holds a reference to a surface to present on */
		VkPresentInfoKHR presentInfo;
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &pje::context.semaphoreRenderingFinished;		// waiting for rendering to finish before presenting
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &pje::context.swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;												// error code from different swapchains possible

		pje::context.result = vkQueuePresentKHR(pje::context.queueForPrototyping, &presentInfo);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkQueuePresentKHR" << endl;
			return;
		}
	}
}

/* Renders a new frame */
/* Loop :
*  1) Acquire next swapchain image related to some logical device
*  2) Submit CommandBuffer designated for image of swapchain to VkQueue => Rendering
*  3) Present result of VkQueue of logical device
*/
void pje::loopVisualizationOf(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		updateMatrices();
		drawFrameOnSurface();
	}
}

// ################################################################################################################################################################## //
/* ######################## DEBUG FUNCTIONS ######################## */
void setObjectNames(const pje::Context& context) {
	pje::set_object_name(context.logicalDevice, context.fenceRenderFinished, "fenceRenderFinished");
}