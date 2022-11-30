#include <globalFuns.h>

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
void clearShaderStages() {
	pje::context.shaderStageInfos.clear();
}

// ################################################################################################################################################################## //

/* cleanupRealtimeRendering should only destroy context's swapchain when the program will be closed */
void cleanupRealtimeRendering(bool reset = false) {
	/* also automatically done by vkDestroyCommandPool */
	vkFreeCommandBuffers(pje::context.logicalDevice, pje::context.commandPool, pje::context.numberOfImagesInSwapchain, pje::context.commandBuffers.get());
	vkDestroyCommandPool(pje::context.logicalDevice, pje::context.commandPool, nullptr);

	/* CLEANUP => delete for all 'new' initializations */
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		vkDestroyFramebuffer(pje::context.logicalDevice, pje::context.framebuffers[i], nullptr);
		vkDestroyImageView(pje::context.logicalDevice, pje::context.imageViews[i], nullptr);
	}

	/* should ONLY be used by stopVulkan() */
	if (!reset) {
		vkDestroyPipeline(pje::context.logicalDevice, pje::context.pipeline, nullptr);
		vkDestroyRenderPass(pje::context.logicalDevice, pje::context.renderPass, nullptr);
		vkDestroyPipelineLayout(pje::context.logicalDevice, pje::context.pipelineLayout, nullptr);
		vkDestroySwapchainKHR(pje::context.logicalDevice, pje::context.swapchain, nullptr);
	}
}

/* Used in startVulkan() and resetRealtimeRendering() */
void setupRealtimeRendering(bool reset = false) {
	/* swapchainInfo for building a Swapchain */
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.flags = 0;
	swapchainInfo.surface = pje::context.surface;
	swapchainInfo.minImageCount = 2;												// requires AT LEAST 2 (double buffering)
	swapchainInfo.imageFormat = pje::context.outputFormat;							// TODO ( choose dynamically )
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;				// TODO ( choose dynamically )
	swapchainInfo.imageExtent = VkExtent2D{ pje::context.windowWidth, pje::context.windowHeight };
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;						// exclusiv to single queue family at a time
	swapchainInfo.queueFamilyIndexCount = 0;										// enables access across multiple queue families => .imageSharingMode = VK_SHARING_MODE_CONCURRENT
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;				// no additional transformations
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// no see through images
	swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;							// TODO ( choose dynamically )
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
	auto swapchainImages = vector<VkImage>(pje::context.numberOfImagesInSwapchain);

	/* swapchainImages will hold the reference to VkImage(s), BUT to access them there has to be VkImageView(s) */
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
		1,								// levelCount : describes how many MipMap levels exist
		0,								// baseArrayLayer : for VR
		1								// layerCount : for VR
	};

	/* ImageView gives access to images in swapchainImages */
	pje::context.imageViews = make_unique<VkImageView[]>(pje::context.numberOfImagesInSwapchain);

	/* Creates image view for each image in swapchain*/
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		imageViewInfo.image = swapchainImages[i];

		pje::context.result = vkCreateImageView(pje::context.logicalDevice, &imageViewInfo, nullptr, &pje::context.imageViews[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateImageView" << endl;
			throw runtime_error("Error at vkCreateImageView");

		}
	}

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

	/* Shader Pipelines consist out of fixed functions and programmable functions (shaders) */
	/* PIPELINE BUILDING - START */
	if (!reset) {
		/* viewportInfo => combines viewport and scissor for the pipeline */
		VkPipelineViewportStateCreateInfo viewportInfo;
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.pNext = nullptr;
		viewportInfo.flags = 0;
		viewportInfo.viewportCount = 1;							// number of viewport
		viewportInfo.pViewports = &initViewport;
		viewportInfo.scissorCount = 1;							// number of scissors
		viewportInfo.pScissors = &initScissor;

		/* Fixed Function : Input Assembler */
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.pNext = nullptr;
		inputAssemblyInfo.flags = 0;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// how single vertices will be assembled
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		/* Usally sizes of vertices and their attributes are defined here
		 * It has similar tasks to OpenGL's glBufferData(), glVertexAttribPointer() etc. (Vertex Shader => layout in) */
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.pNext = nullptr;
		vertexInputInfo.flags = 0;
		vertexInputInfo.vertexBindingDescriptionCount = 0;		// number of pVertexBindingDescriptionS
		vertexInputInfo.pVertexBindingDescriptions = nullptr;	// binding index for AttributeDescription | stride for sizeof(Vertex) | inputRate to decide between per vertex or per instance
		vertexInputInfo.vertexAttributeDescriptionCount = 0;	// number of pVertexAttribute
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;	// location in shader code | binding is binding index of BindingDescription | format defines vertex datatype | offset defines start of the element

		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.pNext = nullptr;
		rasterizationInfo.flags = 0;
		rasterizationInfo.depthClampEnable = VK_FALSE;					// should vertices outside of the near-far-plane be rendered?
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;			// if true primitives won't be rendered but they may be used for other calculations
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;					// backface culling
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
		colorBlendAttachment.blendEnable = VK_TRUE;
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

		// pipelineLayoutInfo declares LAYOUTS and UNIFORMS for compiled shader code
		VkPipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pNext = nullptr;
		pipelineLayoutInfo.flags = 0;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
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
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;		// precision of the sampling
		multisampleInfo.sampleShadingEnable = VK_FALSE;						// enables sampling
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

		/* ATTACHMENTs */

		// VkAttachmentDescription is held by renderPass for its subpasses (VkAttachmentReference.attachment uses VkAttachmentDescription)
		VkAttachmentDescription attachmentDescription;
		attachmentDescription.flags = 0;
		attachmentDescription.format = pje::context.outputFormat;					// has the same format as the swapchainInfo.imageFormat
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;						// for multisampling
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// what happens with the buffer after loading data
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// what happens with the buffer after storing data
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// for stencilBuffer (discarding certain pixels)
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// buffer as input
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// buffer as output

		VkAttachmentReference attachmentRef;
		attachmentRef.attachment = 0;										// index of attachment description
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// CONVERTING: desc.initialLayout -> ref.layout -> desc.finalLayout

		// each render pass holds 1+ sub render passes
		VkSubpassDescription subpassDescription;
		subpassDescription.flags = 0;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// what kind of computation will be done
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;							// actual vertices that being used for shader computation
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentRef;					// which VkAttachmentDescription (attribute bundle) of Renderpass will be used
		subpassDescription.pResolveAttachments = nullptr;
		subpassDescription.pDepthStencilAttachment = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;

		VkSubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;													// works while there is only one subpass in the pipeline
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// src scope has to finish that state ..
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// .. before dst is allowed to start this state
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = 0;

		/* RENDERPASS */

		/* renderPass represents a single execution of the rendering pipeline */
		VkRenderPassCreateInfo renderPassInfo;
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.flags = 0;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &attachmentDescription;					// for subpassDescription's VkAttachmentReference(s)
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;						// all subpasses of the current render pass
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &subpassDependency;

		/* Creating RenderPass for VkGraphicsPipelineCreateInfo */
		pje::context.result = vkCreateRenderPass(pje::context.logicalDevice, &renderPassInfo, nullptr, &pje::context.renderPass);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateRenderPass" << endl;
			throw runtime_error("Error at vkCreateRenderPass");
		}

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
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;					// parts being changeable without building new pipeline
		pipelineInfo.layout = pje::context.pipelineLayout;
		pipelineInfo.renderPass = pje::context.renderPass;				// hold subpasses and subpass attachments
		pipelineInfo.subpass = 0;										// index of .subpass
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;				// deriving from other pipeline to reduce loading time
		pipelineInfo.basePipelineIndex = -1;							// good coding style => invalid index

		pje::context.result = vkCreateGraphicsPipelines(pje::context.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pje::context.pipeline);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateGraphicsPipelines" << endl;
			throw runtime_error("Error at vkCreateGraphicsPipelines");
		}
	}
	/* PIPELINE BUILDING - END */

	/* Framebuffer => bridge between CommandBuffer and Rendertargets/Image Views (Vulkan communication buffer) */
	pje::context.framebuffers = make_unique<VkFramebuffer[]>(pje::context.numberOfImagesInSwapchain);
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		VkFramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = pje::context.renderPass;
		framebufferInfo.attachmentCount = 1;							// one framebuffer may store multiple imageViews
		framebufferInfo.pAttachments = &(pje::context.imageViews[i]);	// imageView => reference to image in swapchain
		framebufferInfo.width = pje::context.windowWidth;				// dimension
		framebufferInfo.height = pje::context.windowHeight;				// dimension
		framebufferInfo.layers = 1;										// dimension

		pje::context.result = vkCreateFramebuffer(pje::context.logicalDevice, &framebufferInfo, nullptr, &pje::context.framebuffers[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkCreateFramebuffer" << endl;
			throw runtime_error("Error at vkCreateFramebuffer");
		}
	}

	/* VkCommandPool holds CommandBuffer which are necessary to tell Vulkan what to do */
	VkCommandPoolCreateInfo commandPoolInfo;
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = 0;												// reset record of single frame buffer | set to buffer to 'transient' for optimisation
	commandPoolInfo.queueFamilyIndex = pje::context.choosenQueueFamily;		// must be the same as in VkDeviceQueueCreateInfo of the logical device & Queue Family 'VK_QUEUE_GRAPHICS_BIT' must be 1

	pje::context.result = vkCreateCommandPool(pje::context.logicalDevice, &commandPoolInfo, nullptr, &pje::context.commandPool);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateCommandPool" << endl;
		throw runtime_error("Error at vkCreateCommandPool");
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = pje::context.commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// primary => processed directly | secondary => invoked by primary
	commandBufferAllocateInfo.commandBufferCount = pje::context.numberOfImagesInSwapchain;	// each buffer in swapchain needs dedicated command buffer

	pje::context.commandBuffers = make_unique<VkCommandBuffer[]>(pje::context.numberOfImagesInSwapchain);
	pje::context.result = vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, pje::context.commandBuffers.get());
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkAllocateCommandBuffers" << endl;
		throw runtime_error("Error at vkAllocateCommandBuffers");
	}

	/* CommandBuffer RECORDING => SET command inside of the command buffer */

	// Begin Recording
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;	// enables sending to gpu before gpu executes same command buffer
	commandBufferBeginInfo.pInheritanceInfo = nullptr;								// for secondary command buffer (see VkCommandBufferAllocateInfo.level)

	// Telling command buffer which render pass to start
	// outside of for loop => reusable for each command buffer
	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = pje::context.renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { pje::context.windowWidth, pje::context.windowHeight };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &pje::context.clearValueDefault;

	/* RECORDING of CommandBuffers */
	for (uint32_t i = 0; i < pje::context.numberOfImagesInSwapchain; i++) {
		/* START RECORDING */
		pje::context.result = vkBeginCommandBuffer(pje::context.commandBuffers[i], &commandBufferBeginInfo);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkBeginCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[i] << endl;
			throw runtime_error("Error at vkBeginCommandBuffer");
		}

		renderPassBeginInfo.framebuffer = pje::context.framebuffers[i];											// framebuffer that the current command buffer is associated with
		vkCmdBeginRenderPass(pje::context.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);	// connects CommandBuffer to Framebuffer ; CommandBuffer knows RenderPass

		// BIND => decide for an pipeline and use it for graphical calculation
		vkCmdBindPipeline(pje::context.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pje::context.pipeline);

		// SET dynamic states for vkCmdDraw
		VkViewport viewport { 0.0f, 0.0f, pje::context.windowWidth, pje::context.windowHeight, 0.0f, 1.0f };
		vkCmdSetViewport(pje::context.commandBuffers[i], 0, 1, &viewport);
		VkRect2D scissor{ {0, 0}, {pje::context.windowWidth, pje::context.windowHeight} };
		vkCmdSetScissor(pje::context.commandBuffers[i], 0, 1, &scissor);

		// DRAW => drawing on swapchain images
		vkCmdDraw(pje::context.commandBuffers[i], 3, 1, 0, 1);				// TODO (hardcoded for current shader)

		vkCmdEndRenderPass(pje::context.commandBuffers[i]);

		/* END RECORDING */
		pje::context.result = vkEndCommandBuffer(pje::context.commandBuffers[i]);
		if (pje::context.result != VK_SUCCESS) {
			cout << "Error at vkEndCommandBuffer of Command Buffer No.:\t" << pje::context.commandBuffers[i] << endl;
			throw runtime_error("Error at vkEndCommandBuffer");
		}
	}
}

/* GLFW: Callback for GLFWwindowsizefun */
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

	cleanupRealtimeRendering(true);
	setupRealtimeRendering(true);
	vkDestroySwapchainKHR(pje::context.logicalDevice, oldSwapchain, nullptr);
}

// ################################################################################################################################################################## //

/* GLFW : Creates a window | Sets callback functions */
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

	/* Sets callback function on resize */
	glfwSetWindowSizeCallback(pje::context.window, resetRealtimeRendering);

	return 0;
}

/* GLFW : Closes created window */
void pje::stopGlfw3() {
	glfwDestroyWindow(pje::context.window);
	glfwTerminate();

	cout << "[DEBUG] GLFW ressources cleaned up." << endl;
}

// ################################################################################################################################################################## //

/* Vulkan : sets pje::context.choosenPhysicalDevice based on needed attributes */
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

/* Vulkan : pje::context setup */
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
	usedInstanceExtensions.push_back("VK_EXT_debug_utils");
	VkValidationFeaturesEXT validationFeatures{ VkStructureType::VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };

	/* instanceInfo used for the actual Vulkan instance */
	/* current LAYERs and EXTENSIONs:					*/
	/* - Validationlayer && GLFW Extensions				*/
	VkInstanceCreateInfo instanceInfo;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = &validationFeatures;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = static_cast<uint32_t>(usedInstanceLayers.size());
	instanceInfo.ppEnabledLayerNames = usedInstanceLayers.data();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(usedInstanceExtensions.size());
	instanceInfo.ppEnabledExtensionNames = usedInstanceExtensions.data();

	/* Vulkan Loader loads layers */
	pje::context.result = vkCreateInstance(&instanceInfo, nullptr, &pje::context.vulkanInstance);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateInstance" << endl;
		return pje::context.result;
	}

	/* Assign function pointer to extension */
	vef::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(pje::context.vulkanInstance, "vkSetDebugUtilsObjectNameEXT"));

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

	/* LOGICAL GPU reference => ! choose the best GPU for your task ! */
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

	/* Getting Queue of some logical device to assign tasks (CommandBuffer) later */
	vkGetDeviceQueue(pje::context.logicalDevice, 0, 0, &pje::context.queueForPrototyping);

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
	cout << "[DEBUG] Programable Pipeline Stages in pje::shaderStageInfos: " << pje::context.shaderStageInfos.size() << endl;

	/* ############## SETUP FOR REAL TIME RENDERING ############## */

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

	/* Creates signaled fence */
	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

	pje::context.result = vkCreateFence(pje::context.logicalDevice, &fenceInfo, nullptr, &pje::context.fenceRenderFinished);
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at vkCreateFence" << endl;
		return pje::context.result;
	}

	/* DEBUG for debugUtils */
	pje::context.result = pje::set_object_name(pje::context.logicalDevice, pje::context.fenceRenderFinished, "fenceRenderFinished");
	if (pje::context.result != VK_SUCCESS) {
		cout << "Error at pje::set_object" << endl;
		return pje::context.result;
	}

	return 0;
}

/* Vulkan : pje::context cleanup */
void pje::stopVulkan() {
	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(pje::context.logicalDevice);

	/* vkDestroy bottom-up */
	vkDestroyFence(pje::context.logicalDevice, pje::context.fenceRenderFinished, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreSwapchainImageReceived, nullptr);
	vkDestroySemaphore(pje::context.logicalDevice, pje::context.semaphoreRenderingFinished, nullptr);

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

/* Used in loopVisualizationOf() */
/* 
 * drawFrameOnSurface has 3 steps => using semaphores
 * 1) get an image from swapchain
 * 2) render inside of selected image
 * 3) give back image to swapchain to present it
 * 
 *    Fences => CPU semaphore | VkSemaphores => GPU semaphore
 */
void drawFrameOnSurface() {
		/* CPU waits here for signaled fence(s)*/
	if (!pje::context.isWindowMinimized) {
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
		pje::context.result = vkAcquireNextImageKHR(
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
		submitInfo.pWaitDstStageMask = waitStageMask.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &pje::context.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &pje::context.semaphoreRenderingFinished;	// which semaphores will be triggered

		/* Submitting commandBuffer to queue => ACTUAL RENDERING */
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

/* Loop : */
/* 
*  1) Acquire next swapchain image related to some logical device
*  2) Submit CommandBuffer designated for image of swapchain to VkQueue => Rendering
*  3) Present result of VkQueue of logical device
*/
void pje::loopVisualizationOf(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrameOnSurface();
	}
}