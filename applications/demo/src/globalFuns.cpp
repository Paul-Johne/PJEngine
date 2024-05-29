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

/* Searches for a memory type index of the choosen physical device that equals memoryTypeBits and the given flags*/
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

// ################################################################################################################################################################## //

/* Returns a VkBuffer Handle without VkMemory */
VkBuffer createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, const char* debugName = nullptr) {
	VkBuffer buffer(VK_NULL_HANDLE);
	
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

/* Allocates VkMemory for a given VkBuffer */
VkDeviceMemory allocateVkBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags memoryFlags) {
	VkDeviceMemory memory(VK_NULL_HANDLE);

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
	rawPJBuffer.deviceMemory = allocateVkBufferMemory(rawPJBuffer.buffer, memoryFlags);
	rawPJBuffer.size = size;
	rawPJBuffer.flags = memoryFlags;

	vkBindBufferMemory(pje::context.logicalDevice, rawPJBuffer.buffer, rawPJBuffer.deviceMemory, 0);
}

/* Maintains pje::stagingBuffer and writes void* data into local device VkBuffer dst */
void copyToLocalDeviceBuffer(const void* data, VkDeviceSize dataSize, VkBuffer dst, VkDeviceSize dstOffset = 0) {
	/* STAGING STEP => copy to coherent buffer */
	
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

	/* SUBMISSION STEP => copy to local device buffer */

	// static variable will be created once for all executions of the given function
	static VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;

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
	vkQueueSubmit(pje::context.queueForPrototyping, 1, &submitInfo, pje::context.fenceSetupTasks);

	/* Waits until CommandBuffers were submitted */
	vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks, VK_TRUE, numeric_limits<uint64_t>::max());
	vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks);
}

/* Sends data on RAM as PJBuffer to [VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT]-VRAM */
void sendPJModelToVRAM(pje::PJBuffer& rawVertexBuffer, pje::PJBuffer& rawIndexBuffer, const pje::PJModel& objectTarget) {
	VkDeviceSize vertexBufferSize = 0;
	VkDeviceSize indexBufferSize = 0;

	for (const auto& mesh : objectTarget.m_meshes) {
		vertexBufferSize += (sizeof(pje::PJMesh::vertexType) * mesh.m_vertices.size());
		indexBufferSize += (sizeof(pje::PJMesh::indexType) * mesh.m_indices.size());
	}

	createPJBuffer(
		rawVertexBuffer,
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,	// used as destination of transfer command
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"vertexBuffer"
	);

	createPJBuffer(
		rawIndexBuffer,
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,	// used as destination of transfer command
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		"indexBuffer"
	);

	VkDeviceSize vByteOffset(0);
	VkDeviceSize iByteOffset(0);

	for (const auto& mesh : objectTarget.m_meshes) {
		VkDeviceSize vByteSize(sizeof(pje::PJMesh::vertexType) * mesh.m_vertices.size());
		VkDeviceSize iByteSize(sizeof(pje::PJMesh::indexType) * mesh.m_indices.size());

		copyToLocalDeviceBuffer(
			mesh.m_vertices.data(),
			vByteSize,
			rawVertexBuffer.buffer,
			vByteOffset
		);
		copyToLocalDeviceBuffer(
			mesh.m_indices.data(),
			iByteSize,
			rawIndexBuffer.buffer,
			iByteOffset
		);

		vByteOffset += vByteSize;
		iByteOffset += iByteSize;
	}
}

// ################################################################################################################################################################## //

/* Returns a VkImage Handle without VkMemory */
VkImage createVkImage(VkFormat format, VkExtent3D imageSize, VkSampleCountFlagBits sampleRate, VkImageUsageFlags usageFlags, uint32_t mipLevels = 1) {
	VkImage image(VK_NULL_HANDLE);
	
	VkImageCreateInfo imageInfo;
	imageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = imageSize;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = sampleRate;
	imageInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usageFlags;
	imageInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

	vkCreateImage(pje::context.logicalDevice, &imageInfo, nullptr, &image);
	return image;
}

/* Allocates VkMemory for a given VkImage */
VkDeviceMemory allocateVkImageMemory(VkImage image, VkMemoryPropertyFlags memoryFlags) {
	VkDeviceMemory			memory(VK_NULL_HANDLE);
	VkMemoryRequirements	memReq;
	VkMemoryAllocateInfo	memAllocInfo;

	/* Considers VK_SAMPLE_COUNT_n_BIT for memReq.size */
	vkGetImageMemoryRequirements(pje::context.logicalDevice, image, &memReq);

	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = memReq.size;
	memAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReq.memoryTypeBits, memoryFlags);

	pje::context.result = vkAllocateMemory(pje::context.logicalDevice, &memAllocInfo, nullptr, &memory);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at allocateVkImageMemory::vkAllocateMemory" << endl;
		throw runtime_error("Error at allocateVkImageMemory::vkAllocateMemory");
	}

	return memory;
}

/* Returns a VkSampler Handle for a combined image sampler */
VkSampler createTexSampler() {
	static VkSampler sampler(VK_NULL_HANDLE);

	VkSamplerCreateInfo texSamplerInfo;
	texSamplerInfo.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	texSamplerInfo.pNext					= nullptr;
	texSamplerInfo.flags					= 0;
	texSamplerInfo.magFilter				= VK_FILTER_NEAREST;						// important for pixel look !
	texSamplerInfo.minFilter				= VK_FILTER_NEAREST;						// important for pixel look !
	texSamplerInfo.addressModeU				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	texSamplerInfo.addressModeV				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	texSamplerInfo.addressModeW				= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	if (pje::config::enableAnisotropy)
		texSamplerInfo.anisotropyEnable		= VK_TRUE;
	else
		texSamplerInfo.anisotropyEnable		= VK_FALSE;
	texSamplerInfo.maxAnisotropy			= 2.0f;										// TODO(civ => 2x anisotropic filtering)
	texSamplerInfo.borderColor				= VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	texSamplerInfo.unnormalizedCoordinates	= VK_FALSE;
	texSamplerInfo.compareEnable			= VK_FALSE;
	texSamplerInfo.compareOp				= VkCompareOp::VK_COMPARE_OP_ALWAYS;
	texSamplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;			// TODO(for later use)
	texSamplerInfo.mipLodBias				= 0.0f;
	texSamplerInfo.minLod					= 0.0f;
	texSamplerInfo.maxLod					= VK_LOD_CLAMP_NONE;						// all mipmap levels ought to be used

	vkCreateSampler(pje::context.logicalDevice, &texSamplerInfo, nullptr, &sampler);

	return sampler;
}

/* Creates and binds VkImage and VkMemory of some pje::PJImage for SAMPLING */
void createPJImage(pje::PJImage& rawImage, unsigned int baseWidth, unsigned int baseHeight, bool genMipmaps = false) {
	// calculates max mipmap levels of images whose ranges are power of 2
	// Example : 128 x 150 => mipCount = 8 => 1 base level & 7 mip levels BUT 8 mip levels are required to reach 1 x 1 image size
	rawImage.mipCount = genMipmaps ? std::max<unsigned int>(std::log2(baseWidth) + 1, std::log2(baseHeight) + 1) : 1;
	
	rawImage.image = createVkImage(
		pje::config::outputFormat,
		VkExtent3D{
			baseWidth,
			baseHeight,
			1
		},
		pje::config::plainImageFactor,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		rawImage.mipCount
	);
	rawImage.deviceMemory = allocateVkImageMemory(
		rawImage.image,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	vkBindImageMemory(pje::context.logicalDevice, rawImage.image, rawImage.deviceMemory, 0);

	VkImageViewCreateInfo viewInfo;
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = rawImage.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = pje::config::outputFormat;
	viewInfo.components = VkComponentMapping{ 
		VK_COMPONENT_SWIZZLE_B, 
		VK_COMPONENT_SWIZZLE_G, 
		VK_COMPONENT_SWIZZLE_R, 
		VK_COMPONENT_SWIZZLE_A 
	};
	viewInfo.subresourceRange = VkImageSubresourceRange{
		VK_IMAGE_ASPECT_COLOR_BIT,
		0, rawImage.mipCount, 0, 1							// mipLevel and array details
	};

	vkCreateImageView(pje::context.logicalDevice, &viewInfo, nullptr, &rawImage.imageView);
}

/* Maintains pje::stagingBuffer and writes void* data into local device VkImage dst */
void copyToLocalDeviceImage(const void* data, pje::TextureInfo texInfo, VkImage dst, VkImageLayout currentDstLayout = VK_IMAGE_LAYOUT_UNDEFINED) {
	/* STAGING STEP => copy to coherent buffer */
	
	if (pje::stagingBuffer.buffer == VK_NULL_HANDLE || pje::stagingBuffer.size < texInfo.size) {
		if (pje::stagingBuffer.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(pje::context.logicalDevice, pje::stagingBuffer.buffer, nullptr);
			vkFreeMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory, nullptr);
		}

		createPJBuffer(
			pje::stagingBuffer,
			texInfo.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			"stagingBuffer"
		);
	}

	/* Staging of PJModel.m_uncompressedTextures[textureID] */
	void* stagingPointer;
	vkMapMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory, 0, VK_WHOLE_SIZE, 0, &stagingPointer);
	memcpy(stagingPointer, data, texInfo.size);
	vkUnmapMemory(pje::context.logicalDevice, pje::stagingBuffer.deviceMemory);

	/* SUBMISSION STEP => copy to local device buffer */

	static VkCommandBuffer stagingCommandBuffer = VK_NULL_HANDLE;

	if (stagingCommandBuffer == VK_NULL_HANDLE) {
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext					= nullptr;
		commandBufferAllocateInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool			= pje::context.commandPool;
		commandBufferAllocateInfo.commandBufferCount	= 1;

		vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, &stagingCommandBuffer);
	}
	else {
		vkResetCommandBuffer(stagingCommandBuffer, 0);
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	/* START - RECORDING */
	vkBeginCommandBuffer(stagingCommandBuffer, &commandBufferBeginInfo);

	VkImageSubresourceRange imageRange{
		VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask
		0,								// baseMipLevel
		VK_REMAINING_MIP_LEVELS,		// levelCount => VK_REMAINING_MIP_LEVELS are all including baseMipLevel
		0,								// baseArrayLayer
		1								// layerCount
	};

	/* srcAccessMask => FLUSH | dstAccessMask => INVALIDATE */

	VkImageMemoryBarrier memBarrier;
	memBarrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier.pNext				= nullptr;
	memBarrier.oldLayout			= currentDstLayout;
	memBarrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	memBarrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;					// for ownership change
	memBarrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;					// for ownership change
	memBarrier.image				= dst;
	memBarrier.subresourceRange		= imageRange;
	memBarrier.srcAccessMask		= VK_ACCESS_NONE;							// sync cache access (what must happen before the barrier in current pipeline stage)
	memBarrier.dstAccessMask		= VK_ACCESS_NONE;							// sync cache access (which operation must wait for the resource)
	
	/* Transition 1/2 */
	vkCmdPipelineBarrier(
		stagingCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,	// pipeline stage in which the barrier takes effect
		VK_PIPELINE_STAGE_TRANSFER_BIT,		// pipeline state that has to wait
		0,									// 0 || VK_DEPENDENCY_BY_REGION_BIT => read what's in the VkImage so far
		0, nullptr,							// Memory Barriers
		0, nullptr,							// Buffer Memory Barriers
		1, &memBarrier						// Image Memory Barriers
	);

	VkImageSubresourceLayers imagePart{
		VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask
		0,								// mipLevel
		0,								// baseArrayLayer
		1								// layerCount
	};

	VkBufferImageCopy region1;
	region1.bufferOffset		= 0;
	region1.bufferRowLength		= 0;
	region1.bufferImageHeight	= 0;
	region1.imageSubresource	= imagePart;
	region1.imageOffset			= VkOffset3D{ 0, 0, 0 };
	region1.imageExtent			= VkExtent3D{ 
		static_cast<unsigned int>(texInfo.x_width), static_cast<unsigned int>(texInfo.y_height), 1 
	};

	vkCmdCopyBufferToImage(stagingCommandBuffer, pje::stagingBuffer.buffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region1);

	/* Transition 2/2 */
	memBarrier.oldLayout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	memBarrier.newLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memBarrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
	memBarrier.dstAccessMask	= VK_ACCESS_NONE;

	vkCmdPipelineBarrier(
		stagingCommandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,			// specifies copy command
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,	// which pipeline state has to wait until the resource is available in newLayout
		0,
		0, nullptr,
		0, nullptr,
		1, &memBarrier
	);

	vkEndCommandBuffer(stagingCommandBuffer);
	/* END - RECORDING */

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stagingCommandBuffer;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	/* Submission of VkCommandBuffer onto VkQueue */
	vkQueueSubmit(pje::context.queueForPrototyping, 1, &submitInfo, pje::context.fenceSetupTasks);

	/* Waits until CommandBuffers were submitted => "Flush and Invalidate all" / complete memory barrier */
	vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks, VK_TRUE, numeric_limits<uint64_t>::max());
	vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks);
}

/* Takes [VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL]-Images and produces ALL possible mipmap textures */
void generateMipmaps(pje::PJImage& uploadedTexture, unsigned int baseWidth, unsigned int baseHeight) {
	VkOffset3D sourceResolution{ baseWidth, baseHeight, 1 };
	VkOffset3D targetResolution{};

	static VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	if (commandBuffer == VK_NULL_HANDLE) {
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = pje::context.commandPool;
		commandBufferAllocateInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, &commandBuffer);
	}
	else {
		vkResetCommandBuffer(commandBuffer, 0);
	}

	VkCommandBufferBeginInfo commandInfo{};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	/* RECORDING - START */
	vkBeginCommandBuffer(commandBuffer, &commandInfo);

	/* Assumption : all mipmap levels are in SHADER_READ_OPTIMAL layout before entering loop */
	for (unsigned int level = 0; level < uploadedTexture.mipCount - 1; ++level) {
		unsigned int readLevel	{ level };
		unsigned int writeLevel	{ level + 1 };

		VkImageSubresourceRange imageRangeSrc{
			VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask
			readLevel,						// baseMipLevel
			1,								// levelCount
			0,								// baseArrayLayer
			1								// layerCount
		};

		VkImageSubresourceRange imageRangeDst{
			VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask
			writeLevel,						// baseMipLevel
			1,								// levelCount
			0,								// baseArrayLayer
			1								// layerCount
		};

		/* 2 Memory Barriers (Layout Transition) : imageRangeSource to TRANSFER_SRC | imageRangeDst to TRANSFER_DST */
		std::array<VkImageMemoryBarrier,2> memBarrier;

		memBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memBarrier[0].pNext = nullptr;
		memBarrier[0].oldLayout = level == 0 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		memBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memBarrier[0].image = uploadedTexture.image;
		memBarrier[0].subresourceRange = imageRangeSrc;
		memBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;			// when cache will be written	=> flush to vram
		memBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;			// when cache will be read		=> invalidate

		memBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memBarrier[1].pNext = nullptr;
		memBarrier[1].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memBarrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memBarrier[1].image = uploadedTexture.image;
		memBarrier[1].subresourceRange = imageRangeDst;
		memBarrier[1].srcAccessMask = VK_ACCESS_NONE;
		memBarrier[1].dstAccessMask = VK_ACCESS_NONE;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,		// next transfer stage (next iteration)
			0,									// 0 || VK_DEPENDENCY_BY_REGION_BIT
			0, nullptr,							// Memory Barriers
			0, nullptr,							// Buffer Memory Barriers
			2, memBarrier.data()				// Image Memory Barriers
		);

		targetResolution.x = std::max<int32_t>(sourceResolution.x / 2, 1);
		targetResolution.y = std::max<int32_t>(sourceResolution.y / 2, 1);
		targetResolution.z = 1;

		/* Defines Image Blit : Src Image + miplevel to read from | Dst Image + mipLevel to write in*/
		VkImageBlit blit{ 
			VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, readLevel, 0, 1 },
			{ {0, 0, 0}, {sourceResolution} },
			VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, writeLevel, 0, 1 },
			{ {0, 0, 0}, {targetResolution} }
		};

		/* Image Blit Command */
		vkCmdBlitImage(
			commandBuffer, 
			uploadedTexture.image, 
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			uploadedTexture.image, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			1, &blit,
			VK_FILTER_LINEAR
		);

		sourceResolution = targetResolution;
	}

	VkImageSubresourceRange imageRange{
			VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask
			0,								// baseMipLevel
			uploadedTexture.mipCount - 1,	// levelCount
			0,								// baseArrayLayer
			1								// layerCount
	};
	
	/* 2 different transitions : 0..maxLevel - 1 TRANSFER_SRC to SHADER_READ | maxLevel TRANSFER_DST to SHADER_READ*/
	std::array<VkImageMemoryBarrier, 2> memBarrier;
	
	memBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier[0].pNext = nullptr;
	memBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	memBarrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier[0].image = uploadedTexture.image;
	memBarrier[0].subresourceRange = imageRange;
	memBarrier[0].srcAccessMask = VK_ACCESS_NONE;
	memBarrier[0].dstAccessMask = VK_ACCESS_NONE;

	imageRange.baseMipLevel = uploadedTexture.mipCount - 1;
	imageRange.levelCount = 1;

	memBarrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier[1].pNext = nullptr;
	memBarrier[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	memBarrier[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memBarrier[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier[1].image = uploadedTexture.image;
	memBarrier[1].subresourceRange = imageRange;
	memBarrier[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memBarrier[1].dstAccessMask = VK_ACCESS_NONE;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,	// next transfer stage (next iteration)
		0,										// 0 || VK_DEPENDENCY_BY_REGION_BIT
		0, nullptr,								// [0] Memory Barriers
		0, nullptr,								// [0] Buffer Memory Barriers
		2, memBarrier.data()					// [2] Image Memory Barriers
	);

	/* RECORDING - END */
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	/* Submission of VkCommandBuffer onto VkQueue */
	vkQueueSubmit(pje::context.queueForPrototyping, 1, &submitInfo, pje::context.fenceSetupTasks);

	/* Waits until CommandBuffers were submitted => "Flush and Invalidate all"/complete memory barrier */
	vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks, VK_TRUE, numeric_limits<uint64_t>::max());
	vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceSetupTasks);
}

/* Sends data on RAM as PJImage to [VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT]-VRAM */
void uploadTextureToVRAM(pje::PJImage& rawImageBuffer, const pje::PJModel& objectTarget, size_t indexOfPJModelTexture, bool genMipmaps = false) {
	auto baseWidth{ static_cast<unsigned int>(objectTarget.m_textureInfos[indexOfPJModelTexture].x_width) };
	auto baseHeight{ static_cast<unsigned int>(objectTarget.m_textureInfos[indexOfPJModelTexture].y_height) };
	
	createPJImage(rawImageBuffer, baseWidth, baseHeight, genMipmaps);
	
	copyToLocalDeviceImage(
		objectTarget.m_uncompressedTextures[indexOfPJModelTexture].data(), 
		objectTarget.m_textureInfos[indexOfPJModelTexture], 
		rawImageBuffer.image
	);

	if (genMipmaps)
		generateMipmaps(rawImageBuffer, baseWidth, baseHeight);

	/* Creates universal VkSampler once */
	if (pje::context.texSampler == VK_NULL_HANDLE) {
		pje::context.texSampler = createTexSampler();
	}
}

/* Sets render targets for pje's Depth and MSAA => pje::rtMsaa and pje::rtDepth */
void createDepthAndMSAA(VkExtent3D imageSize) {
	/* MSAA IMAGE */

	pje::rtMsaa.image = createVkImage(
		pje::config::outputFormat,
		imageSize,
		pje::config::msaaFactor,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	);

	pje::rtMsaa.deviceMemory = allocateVkImageMemory(
		pje::rtMsaa.image,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	vkBindImageMemory(pje::context.logicalDevice, pje::rtMsaa.image, pje::rtMsaa.deviceMemory, 0);

	VkImageViewCreateInfo msaaImageViewInfo;
	msaaImageViewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	msaaImageViewInfo.pNext = nullptr;
	msaaImageViewInfo.flags = 0;
	msaaImageViewInfo.image = pje::rtMsaa.image;
	msaaImageViewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	msaaImageViewInfo.format = pje::config::outputFormat;
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

	pje::context.result = vkCreateImageView(pje::context.logicalDevice, &msaaImageViewInfo, nullptr, &pje::rtMsaa.imageView);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createDepthAndMSAA::vkCreateImageView" << endl;
		throw runtime_error("Error at createDepthAndMSAA::vkCreateImageView");
	}

	/* DEPTH IMAGE */

	pje::rtDepth.image = createVkImage(
		pje::config::depthFormat,
		imageSize,
		pje::config::msaaFactor,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	pje::rtDepth.deviceMemory = allocateVkImageMemory(
		pje::rtDepth.image,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	vkBindImageMemory(pje::context.logicalDevice, pje::rtDepth.image, pje::rtDepth.deviceMemory, 0);

	VkImageViewCreateInfo depthImageViewInfo;
	depthImageViewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthImageViewInfo.pNext = nullptr;
	depthImageViewInfo.flags = 0;
	depthImageViewInfo.image = pje::rtDepth.image;
	depthImageViewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewInfo.format = pje::config::depthFormat;
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

	pje::context.result = vkCreateImageView(pje::context.logicalDevice, &depthImageViewInfo, nullptr, &pje::rtDepth.imageView);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at createDepthAndMSAA::vkCreateImageView" << endl;
		throw runtime_error("Error at createDepthAndMSAA::vkCreateImageView");
	}
}

// ################################################################################################################################################################## //

/* Defines the Layout of VkDescriptorSet which declares needed data in each shader */
void createDescriptorSetLayout(VkDescriptorSetLayout& rawLayout) {
	// SLOT (or binding) of VkDescriptorSetLayout
	array<VkDescriptorSetLayoutBinding, 4> layoutBindings;

	layoutBindings[0].binding = 0;																// layout(set = ?, binding = 0)
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;														// in shader code behind one binding => array[descriptorCount]
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindings[0].pImmutableSamplers = nullptr;

	layoutBindings[1].binding = 1;																// layout(set = ?, binding = 1)
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[1].pImmutableSamplers = nullptr;

	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[2].pImmutableSamplers = nullptr;

	layoutBindings[3].binding = 3;
	layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[3].descriptorCount = 1;
	layoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBindings[3].pImmutableSamplers = nullptr;

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

/* Creates a VkDescriptorPool whose VkDescritorSet-items have 1 uniform, 2 storage buffers and 1 combined image sampler*/
void createDescriptorPool(VkDescriptorPool& rawPool) {
	// order of elements doesn't matter
	auto poolSizes = array{
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
		VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
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
	/* Telling descriptorSet what it is supposed to contain */
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer		= buffer.buffer;						// buffer content on RAM
	bufferInfo.offset		= 0;
	bufferInfo.range		= sizeofData;

	/* Updating descriptors of descriptorSet */
	VkWriteDescriptorSet update;
	update.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	update.pNext			= nullptr;
	update.dstSet			= descriptorSet;
	update.dstBinding		= dstBinding;
	update.dstArrayElement	= 0;									// first element		in array[descriptorCount]
	update.descriptorCount	= 1;									// amount of elements	in array[descriptorCount]
	update.descriptorType	= type;
	update.pImageInfo		= nullptr;
	update.pBufferInfo		= &bufferInfo;							// descriptor's buffer
	update.pTexelBufferView	= nullptr;

	/* Descriptor/Pointer Update */
	vkUpdateDescriptorSets(pje::context.logicalDevice, 1, &update, 0, nullptr);
}

/* Links VkImageView and VkSampler to and existing VkDescriptorSet */
void linkDescriptorSetToImage(VkDescriptorSet& descriptorSet, uint32_t dstBinding, VkDescriptorType type, pje::PJImage& image, VkSampler sampler) {
	/* Telling descriptorSet what it is supposed to contain */
	VkDescriptorImageInfo imageInfo;
	imageInfo.sampler		= sampler;
	imageInfo.imageView		= image.imageView;
	imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;		// layout the texture is currently in

	/* Updating descriptors of descriptorSet */
	VkWriteDescriptorSet update;
	update.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	update.pNext			= nullptr;
	update.dstSet			= descriptorSet;
	update.dstBinding		= dstBinding;
	update.dstArrayElement	= 0;										// first element		in shaderArray[descriptorCount]
	update.descriptorCount	= 1;										// amount of elements	in shaderArray[descriptorCount]
	update.descriptorType	= type;
	update.pImageInfo		= &imageInfo;								// descriptor's image
	update.pBufferInfo		= nullptr;
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

/* Cleanup of VkHandles on reset or on app termination */
void pje::cleanupRealtimeRendering(bool reset) {
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		vkDestroyFramebuffer(pje::context.logicalDevice, pje::context.swapchainFramebuffers[i], nullptr);
		vkDestroyImageView(pje::context.logicalDevice, pje::context.swapchainImageViews[i], nullptr);		// vkDestroyImage for swapchainImages NOT NECESSARY - handled by vkDestroySwapchainKHR
	}

	/* I definitely need vma.. */
	vkDestroyImageView(pje::context.logicalDevice, pje::rtDepth.imageView, nullptr);
	vkFreeMemory(pje::context.logicalDevice, pje::rtDepth.deviceMemory, nullptr);
	vkDestroyImage(pje::context.logicalDevice, pje::rtDepth.image, nullptr);
	vkDestroyImageView(pje::context.logicalDevice, pje::rtMsaa.imageView, nullptr);
	vkFreeMemory(pje::context.logicalDevice, pje::rtMsaa.deviceMemory, nullptr);
	vkDestroyImage(pje::context.logicalDevice, pje::rtMsaa.image, nullptr);

	/* should ONLY be used by stopVulkan() */
	if (!reset) {
		vkDestroySampler(pje::context.logicalDevice, pje::context.texSampler, nullptr);

		vkDestroyImageView(pje::context.logicalDevice, pje::texAlbedo.imageView, nullptr);
		vkFreeMemory(pje::context.logicalDevice, pje::texAlbedo.deviceMemory, nullptr);
		vkDestroyImage(pje::context.logicalDevice, pje::texAlbedo.image, nullptr);

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

/* Recording of context.commandBuffer - Declares what has to be rendered */
void recordRenderCommand(pje::PJModel& renderable, uint32_t imageIndex) {
	// Defines CommandBuffer behavior
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;		// usage behavior for command buffer
	commandBufferBeginInfo.pInheritanceInfo = nullptr;								// for secondary command buffer (see VkCommandBufferAllocateInfo.level)

	array<VkClearValue, 3> clearValues{};
	clearValues[0].color = VkClearColorValue{ 
		pje::config::clearValueDefault[0], 
		pje::config::clearValueDefault[1], 
		pje::config::clearValueDefault[2],
		pje::config::clearValueDefault[3]
	};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[2].color = VkClearColorValue{										// clear value for Resolve Image is probably useless
		pje::config::clearValueDefault[0],
		pje::config::clearValueDefault[1],
		pje::config::clearValueDefault[2],
		pje::config::clearValueDefault[3]
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

	/* RECORDING of context.commandBuffers[aquiredImageIndex] */
	/* START RECORDING */
	pje::context.result = vkBeginCommandBuffer(pje::context.commandBuffers[imageIndex], &commandBufferBeginInfo);		// resets CommandBuffer automatically
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkBeginCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[imageIndex] << endl;
		throw runtime_error("Error at vkBeginCommandBuffer");
	}

	renderPassBeginInfo.framebuffer = pje::context.swapchainFramebuffers[imageIndex];									// framebuffer that the current command buffer is associated with
	vkCmdBeginRenderPass(pje::context.commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);	// connects CommandBuffer to Framebuffer ; CommandBuffer knows RenderPass

	// BIND => decide for an pipeline and use it for graphical calculation
	vkCmdBindPipeline(pje::context.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pje::context.pipeline);

	// SET dynamic states for vkCmdDraw
	VkViewport viewport{ 0.0f, 0.0f, pje::context.windowWidth, pje::context.windowHeight, 0.0f, 1.0f };
	vkCmdSetViewport(pje::context.commandBuffers[imageIndex], 0, 1, &viewport);
	VkRect2D scissor{ {0, 0}, {pje::context.windowWidth, pje::context.windowHeight} };
	vkCmdSetScissor(pje::context.commandBuffers[imageIndex], 0, 1, &scissor);

	// BIND => ordering to use certain buffer in VRAM
	array<VkDeviceSize, 1> offsets{ 0 };
	vkCmdBindDescriptorSets(
		pje::context.commandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pje::context.pipelineLayout,
		0,
		1,
		&pje::context.descriptorSet,
		0,
		nullptr
	);

	vkCmdBindVertexBuffers(pje::context.commandBuffers[imageIndex], 0, 1, &pje::vertexBuffer.buffer, offsets.data());
	vkCmdBindIndexBuffer(pje::context.commandBuffers[imageIndex], pje::indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	// DRAW => drawing on swapchain images
	for (const auto& mesh : renderable.m_meshes) {
		vkCmdDrawIndexed(pje::context.commandBuffers[imageIndex], mesh.m_indices.size(), 1, mesh.m_offsetIndices, mesh.m_offsetVertices, 0);
	}

#ifdef ACTIVATE_ENGINE_GUI
	/* additional EngineGui object created once for all function invocations */
	static std::unique_ptr<pje::EngineGui> const& gui = std::unique_ptr<pje::EngineGui>{};
	gui->drawGui(imageIndex);
#endif

	vkCmdEndRenderPass(pje::context.commandBuffers[imageIndex]);

	pje::context.result = vkEndCommandBuffer(pje::context.commandBuffers[imageIndex]);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkEndCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[imageIndex] << endl;
		throw runtime_error("Error at vkEndCommandBuffer");
	}
	/* END RECORDING */
}

/* Setup for Swapchain, Renderpass, Pipeline and CommandBuffer Recording */
void setupRealtimeRendering(bool reset = false) {
	/* swapchainInfo for building a Swapchain */
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.flags = 0;
	swapchainInfo.surface = pje::context.surface;									// vkQueuePresentKHR() will present on this surface
	swapchainInfo.minImageCount = pje::config::neededSurfaceImages;					// requires AT LEAST 2 (double buffering)
	swapchainInfo.imageFormat = pje::config::outputFormat;							// VkFormat of rendertargets in swapchain
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;				// specifies supported color space of presentation (surface)
	swapchainInfo.imageExtent = VkExtent2D{ pje::context.windowWidth, pje::context.windowHeight };
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					// suitable for a resolve attachment in VkFramebuffer
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;						// exclusiv to single queue family at a time
	swapchainInfo.queueFamilyIndexCount = 0;										// enables access across multiple queue families => .imageSharingMode = VK_SHARING_MODE_CONCURRENT
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;				// no additional transformations
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// no see through images
	if (pje::config::enableVSync)
		swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;						// VK_PRESENT_MODE_FIFO_KHR			=> VSYNC
	else
		swapchainInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;					// VK_PRESENT_MODE_IMMEDIATE_KHR	=> UNLIMITED FPS
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
	
	VkImageViewCreateInfo imageViewInfo;
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = pje::config::outputFormat;
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
	/* Creates Rendertargets Depth and MSAA */
	createDepthAndMSAA(VkExtent3D{ pje::context.windowWidth, pje::context.windowHeight, 1 });

	if (!reset) {
		/* RENDERPASS - START */

		// VkAttachmentDescription is held by renderPass for its subpasses (VkAttachmentReference.attachment uses VkAttachmentDescription)
		VkAttachmentDescription attachmentDescriptionMSAA;
		attachmentDescriptionMSAA.flags = 0;
		attachmentDescriptionMSAA.format = pje::config::outputFormat;
		attachmentDescriptionMSAA.samples = pje::config::msaaFactor;
		attachmentDescriptionMSAA.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptionMSAA.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptionMSAA.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;			// ignored when attachment doesn't contain stencil component
		attachmentDescriptionMSAA.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;		// ignored when attachment doesn't contain stencil component
		attachmentDescriptionMSAA.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptionMSAA.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachmentDescriptionDepth;
		attachmentDescriptionDepth.flags = 0;
		attachmentDescriptionDepth.format = pje::config::depthFormat;						// no stencil component
		attachmentDescriptionDepth.samples = pje::config::msaaFactor;
		attachmentDescriptionDepth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptionDepth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptionDepth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptionDepth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptionDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptionDepth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachmentDescriptionResolve;
		attachmentDescriptionResolve.flags = 0;
		attachmentDescriptionResolve.format = pje::config::outputFormat;					// has the same format as the swapchainInfo.imageFormat
		attachmentDescriptionResolve.samples = pje::config::plainImageFactor;				// necessary amount samples for this attachment
		attachmentDescriptionResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// what happens with the attachment when renderpass begins
		attachmentDescriptionResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// what happens with the attachment when renderpass ends
		attachmentDescriptionResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
		attachmentDescriptionResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
		attachmentDescriptionResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// buffer as input
		attachmentDescriptionResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;			// buffer as output

		// References are hold by some VkSubpass to reference certain VkAttachmentDescription
		VkAttachmentReference attachmentRefMSAA;
		attachmentRefMSAA.attachment = 0;													// index into VkRenderPassCreateInfo.pAttachments
		attachmentRefMSAA.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference attachmentRefDepth;
		attachmentRefDepth.attachment = 1;													// index into VkRenderPassCreateInfo.pAttachments
		attachmentRefDepth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference attachmentRefResolve;
		attachmentRefResolve.attachment = 2;												// index into VkRenderPassCreateInfo.pAttachments
		attachmentRefResolve.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// CONVERTING: initialLayout -> layout -> finalLayout

		// each VkRenderPass holds 1+ VkSubpasses | VkSubpass equals one run through pipeline
		VkSubpassDescription subpassDescription;
		subpassDescription.flags = 0;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;				// what kind of computation will be done
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentRefMSAA;							// layout(location = 0) out vec4 color;
		subpassDescription.pDepthStencilAttachment = &attachmentRefDepth;					// which VkAttachmentDescription of Renderpass will be used | deactivate => nullptr
		subpassDescription.pResolveAttachments = &attachmentRefResolve;						// -||-
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;										// actual vertices that being used for shader computation

		// VkSubpassDependency can be seen as a semaphore between subpasses for their micro tasks
		VkSubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;									// relates to commands bevor vkCmdBeginRenderPass
		subpassDependency.dstSubpass = 0;													// index of subpass in renderpass
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// src scope has to finish that state ..
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// .. before dst is allowed to start this state
		subpassDependency.srcAccessMask = VK_ACCESS_NONE;
		subpassDependency.dstAccessMask = VK_ACCESS_NONE;
		subpassDependency.dependencyFlags = 0;

		VkSubpassDependency subpassDependencyDepth;
		subpassDependencyDepth.srcSubpass = VK_SUBPASS_EXTERNAL;								// relates to commands bevor vkCmdBeginRenderPass
		subpassDependencyDepth.dstSubpass = 0;
		subpassDependencyDepth.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencyDepth.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencyDepth.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;	// after what point in [srcStage] does the data need to be flushed
		subpassDependencyDepth.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;		// at what point in [dstStage] are new data required, so cache is invalid
		subpassDependencyDepth.dependencyFlags = 0;

		array<VkAttachmentDescription, 3> attachmentDescriptions = {
			attachmentDescriptionMSAA, attachmentDescriptionDepth, attachmentDescriptionResolve
		};

		array<VkSubpassDependency, 2> subpassDependencies = {
			subpassDependencyDepth, subpassDependency
		};

		/* renderPass represents a single execution of the rendering pipeline */
		VkRenderPassCreateInfo renderPassInfo;
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.flags = 0;
		renderPassInfo.attachmentCount = 3;										// all VkAttachmentDescription(s) for subpassDescription's VkAttachmentReference(s)
		renderPassInfo.pAttachments = attachmentDescriptions.data();			// order	DOES	matter ! (connected to order of framebuffer.pAttachments)
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;						// all subpasses of the current render pass
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = subpassDependencies.data();				// order	DOESN'T	matter !

		/* Creating RenderPass for VkGraphicsPipelineCreateInfo */
		pje::context.result = vkCreateRenderPass(pje::context.logicalDevice, &renderPassInfo, nullptr, &pje::context.renderPass);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateRenderPass" << endl;
			throw runtime_error("Error at vkCreateRenderPass");
		}

		/* RENDERPASS - END */

		/* PIPELINE BUILDING - START */

		/* Viewport */
		VkViewport initViewport;						// buffer to display transformation
		initViewport.x = 0.0f;							// upper left corner
		initViewport.y = 0.0f;							// upper left corner
		initViewport.width = pje::context.windowWidth;
		initViewport.height = pje::context.windowHeight;
		initViewport.minDepth = 0.0f;
		initViewport.maxDepth = 1.0f;

		/* Scissor */
		VkRect2D initScissor;							// in which area pixels are stored in buffer
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
		 * It has similar tasks to OpenGL's glBufferData(), glVertexAttribPointer() => (Vertex Shader => layout in) */
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

		// pipelineLayoutInfo declares LAYOUTS and UNIFORMS (s. VkDescriptorSet) for compiled shader code
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
		multisampleInfo.rasterizationSamples = pje::config::msaaFactor;		// precision of the sampling
		multisampleInfo.sampleShadingEnable = VK_FALSE;						// enables super sampling
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
		depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateInfo.pNext = nullptr;
		depthStencilStateInfo.flags = 0;
#ifdef ACTIVATE_DEPTH_TEST
		depthStencilStateInfo.depthTestEnable = VK_TRUE;
#else
		depthStencilStateInfo.depthTestEnable = VK_FALSE;
#endif
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

	/* VkCommandPool holds CommandBuffer which are necessary to tell GPU what to do */
	if (!reset) {
		VkCommandPoolCreateInfo commandPoolInfo;
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.pNext = nullptr;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;	// reset record of single frame buffer | set to 'transient' for optimisation
		commandPoolInfo.queueFamilyIndex = pje::context.choosenQueueFamily;			// must be the same as in VkDeviceQueueCreateInfo of the logical device & Queue Family 'VK_QUEUE_GRAPHICS_BIT' must be 1

		pje::context.result = vkCreateCommandPool(pje::context.logicalDevice, &commandPoolInfo, nullptr, &pje::context.commandPool);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateCommandPool" << endl;
			throw runtime_error("Error at vkCreateCommandPool");
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
			pje::rtMsaa.imageView,
			pje::rtDepth.imageView,
			pje::context.swapchainImageViews[i]
		};

		// VkAttachmentDescription of VkRenderPass describes rendertargets => ORDER IS VERY IMPORTART IN .data() !
		framebufferInfo.pAttachments = rendertargets.data();

		pje::context.result = vkCreateFramebuffer(pje::context.logicalDevice, &framebufferInfo, nullptr, &pje::context.swapchainFramebuffers[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateFramebuffer" << endl;
			throw runtime_error("Error at vkCreateFramebuffer");
		}
	}

	/* Shader Ressources - START */
	if (!reset) {
		/* Allocation of VkBuffer to transfer CPU vertices/indices onto GPU */
		sendPJModelToVRAM(pje::vertexBuffer, pje::indexBuffer, pje::loadedModels[pje::config::selectedPJModel]);

		/* Allocation of PJImage to transfer CPU uncompressed texels onto GPU */
		uploadTextureToVRAM(pje::texAlbedo, pje::loadedModels[pje::config::selectedPJModel], 0, true);

		/* Allocation of COHERENT VkBuffer to transfer uniforms onto GPU */
		createUniformBuffer(pje::uniformsBuffer, sizeof(pje::Uniforms), "uniformsBuffer");

		/* Allocation - TEMPORARY - Bone Dummy without useful data inside */
		createPJBuffer(pje::storeBoneRefs, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "storeBoneRefs");
		createPJBuffer(pje::storeBoneMatrices, 1, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "storeBoneMatrices");
	}

	if (!reset) {
		/* Allocation of VkDescriptorPool und VkDescriptorSet to update uniforms */
		createDescriptorPool(pje::context.descriptorPool);
		createDescriptorSet(pje::context.descriptorSet);

		/* Linkage of Uniforms and Storage Buffers to DescriptorSet*/
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, pje::uniformsBuffer);
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pje::storeBoneRefs);
		linkDescriptorSetToBuffer(pje::context.descriptorSet, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pje::storeBoneMatrices);
		linkDescriptorSetToImage(pje::context.descriptorSet, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pje::texAlbedo, pje::context.texSampler);
	}
	/* Shader Ressources - END */
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

	/* Prevents out of bounds size */
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

	// Fullscreen Mode : glfwGetPrimaryMonitor() instead of nullptr
	pje::context.window = glfwCreateWindow(pje::context.windowWidth, pje::context.windowHeight, windowName, nullptr, nullptr);
	if (pje::context.window == NULL) {
		cout << "Error at glfwCreateWindow" << endl;
		glfwTerminate();
		return -1;
	}

	// glfwSetInputMode(pje::context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	setGlfwCallbacks();

	return 0;
}

/* Closes created window */
void pje::stopGlfw3() {
	glfwDestroyWindow(pje::context.window);
	glfwTerminate();

	cout << "[GLFW] GLFW ressources cleaned up." << endl;
}

// ################################################################################################################################################################## //

/* Sets pje::context.choosenPhysicalDevice based on needed attributes */
void selectGPU(const vector<VkPhysicalDevice>& physicalDevices, VkPhysicalDeviceType preferredType, const vector<VkQueueFlagBits>& neededFamilyQueueAttributes, uint32_t neededSurfaceImages, VkFormat imageColorFormat, VkPresentModeKHR neededPresentationMode) {
	if (physicalDevices.size() == 0)
		throw runtime_error("[ERROR] No GPU was found on this computer.");
	
	const uint8_t AMOUNT_OF_REQUIREMENTS = 5;
	uint8_t requirementCounter = 0;
	uint32_t devicesCounter = 0;

	for (const auto& physicalDevice : physicalDevices) {
		requirementCounter = 0;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		cout << "\n[PJE] GPU [" << props.deviceName << "] was found." << endl;
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			cout << "[PJE] \tGPU is VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU." << endl;
		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			cout << "[PJE] \tGPU is VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU." << endl;

		if (props.deviceType == preferredType) {
			++requirementCounter;
			cout << "[PJE] \tPreferred device type was found." << endl;
		}
		else {
			cout << "[PJE] \tPreferred device type wasn't found." << endl;
			cout << "[PJE] \tPJEngine declined " << props.deviceName << ".\n" << endl;
			++devicesCounter;
			continue;
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
				cout << "[PJE] \tNo suitable queue family was found." << endl;
				cout << "[PJE] \tPJEngine declined " << props.deviceName << ".\n" << endl;
				++devicesCounter;
				continue;
			}
			else {
				++requirementCounter;
				cout << "[PJE] \tpje::context.choosenQueueFamily was set to : " << pje::context.choosenQueueFamily << endl;
			}
		}

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pje::context.surface, &surfaceCapabilities);

			if (surfaceCapabilities.minImageCount >= neededSurfaceImages) {
				++requirementCounter;
				cout << "[PJE] \tPhysical device supports needed amount of surface images" << endl;
			}
			else {
				cout << "[PJE] \tPhysical device cannot provide the needed amount of surface images" << endl;
				cout << "[PJE] \tPJEngine declined " << props.deviceName << ".\n" << endl;
				++devicesCounter;
				continue;
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
				cout << "[PJE] \tRequired VkFormat wasn't provided by the physical device" << endl;
				cout << "[PJE] \tPJEngine declined " << props.deviceName << ".\n" << endl;
				++devicesCounter;
				continue;
			}
			else {
				++requirementCounter;
				cout << "[PJE] \tPhysical device supports required VkFormat" << endl;
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
				cout << "[PJE] \tRequired VkPresentModeKHR wasn't provided by the physical device" << endl;
				cout << "[PJE] \tPJEngine declined " << props.deviceName << ".\n" << endl;
				++devicesCounter;
				continue;
			}
			else {
				++requirementCounter;
				cout << "[PJE] \tPhysical device supports required VkPresentModeKHR" << endl;
			}
		}

		if (requirementCounter == AMOUNT_OF_REQUIREMENTS) {
			cout << "[PJE] \tPJEngine is choosing " << props.deviceName << ".\n" << endl;

			pje::context.choosenPhysicalDevice = devicesCounter;
			pje::context.windowWidth = surfaceCapabilities.currentExtent.width;
			pje::context.windowHeight = surfaceCapabilities.currentExtent.height;

			return;
		}
		else {
			cout << "[ERROR] \tPJEngine declined. This part should not be reachable..\n" << endl;
			++devicesCounter;
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
	appInfo.pApplicationName = pje::config::appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PJ Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	cout << "[PJE] Engine started.." << endl;

	/* LAYERs act like hooks to intercept Vulkan API calls */
	uint32_t numberOfInstanceLayers = 0;
	vkEnumerateInstanceLayerProperties(&numberOfInstanceLayers, nullptr);

	auto availableInstanceLayers = vector<VkLayerProperties>(numberOfInstanceLayers);
	vkEnumerateInstanceLayerProperties(&numberOfInstanceLayers, availableInstanceLayers.data());

	cout << "\n[PJE] Available Instance Layers:\t" << numberOfInstanceLayers << endl;
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
#ifndef NDEBUG
		return -1;
#endif
	}

	/* EXTENSIONS deliver optional functionalities provided by layers, loader or ICD */
	uint32_t numberOfInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfInstanceExtensions, nullptr);

	auto availableInstanceExtensions = vector<VkExtensionProperties>(numberOfInstanceExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfInstanceExtensions, availableInstanceExtensions.data());

	cout << "\n[PJE] Available Instance Extensions:\t" << numberOfInstanceExtensions << endl;
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

	/* Adds and configures validation (extension) features */
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

	/* Query extension functions of VkInstance by using vkGetInstanceProcAddr */
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

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter):
		2) get reference for a number of GPUs and store them in an array of physical devices */
	vkEnumeratePhysicalDevices(pje::context.vulkanInstance, &numberOfPhysicalDevices, pje::context.physicalDevices.data());

	cout << "\n[PJE] Number of GPUs:\t\t" << numberOfPhysicalDevices << endl;

	/* Shows some properties and features for each available GPU */
	for (uint32_t i = 0; i < numberOfPhysicalDevices; i++) {
		pje::debugPhysicalDeviceStats(pje::context.physicalDevices[i]);
	}

	/* ########## Decides on the right GPU and QueueFamily for PJEngine ########## */
	VkPresentModeKHR neededPresentationMode;
	if (pje::config::enableVSync)
		neededPresentationMode = VK_PRESENT_MODE_FIFO_KHR;		// VK_PRESENT_MODE_FIFO_KHR			=> VSYNC
	else
		neededPresentationMode = VK_PRESENT_MODE_IMMEDIATE_KHR;	// VK_PRESENT_MODE_IMMEDIATE_KHR	=> UNLIMITED FPS

	/* Sets pje::context.choosenPhysicalDevice and pje::context.choosenQueueFamily */
	selectGPU(
		pje::context.physicalDevices,
		pje::config::preferredPhysicalDeviceType,
		vector<VkQueueFlagBits>(pje::config::neededFamilyQueueAttributes.begin(), pje::config::neededFamilyQueueAttributes.end()),
		pje::config::neededSurfaceImages,
		pje::config::outputFormat,
		neededPresentationMode
	);

	array<float, 1> queuePriorities { 1.0f };

	/* deviceQueueCreateInfo used for deviceInfo => choose ammount of queue of a certain queue family */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext = nullptr;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = pje::context.choosenQueueFamily;
	deviceQueueInfo.queueCount = 1;
	deviceQueueInfo.pQueuePriorities = queuePriorities.data();

	/* EXTENSIONS on device level */
	auto deviceExtensions = array { "VK_KHR_swapchain" };					// Swapchains => Real Time Rendering
	
	/* all device features for Vulkan 1.1 and upwards */
	VkPhysicalDeviceFeatures2 coreDeviceFeature { VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

	/* deviceInfo declares what resources will be claimed */
	/* deviceInfo is necessary for a logical reference    */
	VkDeviceCreateInfo deviceInfo;
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &coreDeviceFeature;									// OR nullptr							[ for Vulkan 1.0 ]
	deviceInfo.flags = 0;
	deviceInfo.queueCreateInfoCount = 1;									// number of VkDeviceQueueCreateInfo (1+ possible)
	deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceInfo.pEnabledFeatures = nullptr;									// OR context.physicalDeviceFeatures	[ for Vulkan 1.0 ]

	if (pje::config::enableAnisotropy) {
		pje::context.physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
	}
	coreDeviceFeature.features = pje::context.physicalDeviceFeatures;

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
	cout << "\n[PJE] Basic Shader Bytes\n"
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
	cout << "[PJE] Programmable Pipeline Stages\n"
		"\tpje::shaderStageInfos: " << pje::context.shaderStageInfos.size() << endl;

	/* ############## SETUP SEMAPHORES AND FENCES ############## */

	/* Creates unsignaled semaphores for signaling that a batch of commands has been processed */
	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;								// future use

	vkCreateSemaphore(pje::context.logicalDevice, &semaphoreInfo, nullptr, &pje::context.semaphoreSwapchainImageReceived);
	vkCreateSemaphore(pje::context.logicalDevice, &semaphoreInfo, nullptr, &pje::context.semaphoreRenderingFinished);

	/* Creates unsignaled fences for image rendering and setup tasks */
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;

	fenceInfo.flags = 0;									// unsignaled
	vkCreateFence(pje::context.logicalDevice, &fenceInfo, nullptr, &pje::context.fenceSetupTasks);
	fenceInfo.flags = 1;									// signaled
	vkCreateFence(pje::context.logicalDevice, &fenceInfo, nullptr, &pje::context.fenceRenderFinished);

	/* ############## SETUP FOR REAL TIME RENDERING ############## */
	setupRealtimeRendering();

	/* ############## DEBUG for debugUtils ############## */
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
	vkDestroyFence(pje::context.logicalDevice, pje::context.fenceSetupTasks, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreSwapchainImageReceived, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreRenderingFinished, nullptr);

	/* vkDestroy, vkFree etc. */
	cleanupRealtimeRendering();

	vkDestroyShaderModule(pje::context.logicalDevice, pje::context.shaderModuleBasicVert, nullptr);
	vkDestroyShaderModule(pje::context.logicalDevice, pje::context.shaderModuleBasicFrag, nullptr);
	clearShaderStages();

	vkDestroyDevice(pje::context.logicalDevice, nullptr);
	vkDestroySurfaceKHR(pje::context.vulkanInstance, pje::context.surface, nullptr);
	vkDestroyInstance(pje::context.vulkanInstance, nullptr);

	cout << "[PJE] Vulkan API ressources cleaned up." << endl;
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
		duration * glm::radians(-20.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	// current cameraPose => TODO(modifiable via ImGui)
	pje::uniforms.viewMatrix = pje::context.cameraPose;

	pje::uniforms.projectionMatrix = glm::perspective(
		glm::radians(60.0f),
		pje::context.windowWidth / (float)pje::context.windowHeight,
		pje::config::nearPlane,
		pje::config::farPlane
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
		uint32_t imageIndex;								// current imageIndex of swapchain
		vkAcquireNextImageKHR(								// async => GPU can already compute without having a rendertarget
			pje::context.logicalDevice,
			pje::context.swapchain,
			0,												// timeout in ns before abort
			pje::context.semaphoreSwapchainImageReceived,	// semaphore	=> only visible on GPU side
			VK_NULL_HANDLE,									// fences		=> CPU - GPU synchronization
			&imageIndex
		);
		
		/* CPU waits here until all fences in pFences are signaled */
		vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceRenderFinished, VK_TRUE, numeric_limits<uint64_t>::max());
		/* Unsignals fence(s) */
		vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceRenderFinished);
		/* Resets current pje::context.commandBuffer to initial state */
		vkResetCommandBuffer(pje::context.commandBuffers[imageIndex], 0);

		/* Record pje::context.commandBuffer[imageIndex] designated to rendering */
		recordRenderCommand(pje::loadedModels[pje::config::selectedPJModel], imageIndex);

		array<VkPipelineStageFlags, 1> waitStageMask{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	// allows async rendering behavior
		};

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &pje::context.semaphoreSwapchainImageReceived;
		submitInfo.pWaitDstStageMask = waitStageMask.data();						// wait at this pipeline stage until pWaitSemaphore is signaled
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &pje::context.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &pje::context.semaphoreRenderingFinished;	// which semaphore will be triggered when submit step done

		/* Submitting CommandBuffer to queue => ACTUAL RENDERING */
		/* Fence must be unsignaled to proceed and signals fence once ALL submitted command buffers have completed execution */
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

		/* Marks images as presentable */
		pje::context.result = vkQueuePresentKHR(pje::context.queueForPrototyping, &presentInfo);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkQueuePresentKHR" << endl;
			return;
		}
	}
}

/* Renders a new frame for current setup */
/* Loop :
*  1) Acquire next swapchain image related to some logical device
*  2) Submit CommandBuffer designated for image of swapchain to VkQueue => Rendering
*  3) Present result of VkQueue of logical device
*/
void pje::loopVisualizationOf(GLFWwindow* window, std::unique_ptr<pje::EngineGui> const& gui) {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

#ifdef ACTIVATE_ENGINE_GUI
		gui->nextGuiFrame();
#endif

		/* Code for perspective of the camera into the scene */
		updateMatrices();

		/* Records pje::context.commandBuffer[imageIndex] | Draws on surface connected to swapchain */
		drawFrameOnSurface();
	}
}

// ################################################################################################################################################################## //
/* ######################## DEBUG FUNCTIONS ######################## */
void setObjectNames(const pje::Context& context) {
	pje::set_object_name(context.logicalDevice, context.fenceSetupTasks, "fenceSetupTasks");
	pje::set_object_name(context.logicalDevice, context.fenceRenderFinished, "fenceRenderFinished");
}