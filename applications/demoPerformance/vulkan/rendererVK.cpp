#include "rendererVK.h"
#define DEBUG

pje::renderer::RendererVK::RendererVK(const pje::engine::ArgsParser& parser, GLFWwindow* const window) : 
	RendererInterface(), m_context(), m_renderWidth(parser.m_width), m_renderHeight(parser.m_height), 
	m_vsync(parser.m_vsync), m_anisotropyLevel(AnisotropyLevel::TWOx), m_msaaFactor(VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT) {

	std::cout << "[VK] \tVulkan Version: " << getApiVersion() << std::endl;

#ifdef DEBUG
	if (!requestLayer(RequestLevel::Instance, "VK_LAYER_KHRONOS_validation"))
		throw std::runtime_error("Validation layer for instance dispatch chain was not found.");
#endif // DEBUG
	requestGlfwInstanceExtensions();
	setInstance();
	setSurface(window);
	setPhysicalDevice(
		VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
		VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT | VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT,
		m_context.surfaceImageCount,
		std::vector<VkSurfaceFormatKHR>{ 
			{VkFormat::VK_FORMAT_B8G8R8A8_UNORM, VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR}, 
			{VkFormat::VK_FORMAT_R8G8B8A8_UNORM, VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR} 
		}
	);
	if (!requestExtension(RequestLevel::Device, "VK_KHR_swapchain"))
		throw std::runtime_error("Choosen GPU does not support the required swapchain extension.");
	setDeviceAndQueue(VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT | VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT);
	setShaderProgram("basic_vulkan");
	setFence(m_context.fenceSetupTask, false);
	setFence(m_context.fenceImgRendered, true);
	setSemaphore(m_context.semSwapImgReceived);
	setSemaphore(m_context.semImgRendered);
	setDescriptorSet();
	setRenderpass();
	//buildPipeline();
}

pje::renderer::RendererVK::~RendererVK() {
	vkDeviceWaitIdle(m_context.device);
	std::cout << "[VK] \tCleaning up handles ..." << std::endl;

	if (m_context.pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(m_context.device, m_context.pipeline, nullptr);
	if (m_context.pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(m_context.device, m_context.pipelineLayout, nullptr);
	if (m_context.renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(m_context.device, m_context.renderPass, nullptr);
	if (m_context.descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(m_context.device, m_context.descriptorPool, nullptr);	// + freeing <DescriptorSet>
	if (m_context.descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(m_context.device, m_context.descriptorSetLayout, nullptr);
	if (m_context.semImgRendered != VK_NULL_HANDLE)
		vkDestroySemaphore(m_context.device, m_context.semImgRendered, nullptr);
	if (m_context.semSwapImgReceived != VK_NULL_HANDLE)
		vkDestroySemaphore(m_context.device, m_context.semSwapImgReceived, nullptr);
	if (m_context.fenceImgRendered != VK_NULL_HANDLE)
		vkDestroyFence(m_context.device, m_context.fenceImgRendered, nullptr);
	if (m_context.fenceSetupTask != VK_NULL_HANDLE)
		vkDestroyFence(m_context.device, m_context.fenceSetupTask, nullptr);
	if (m_context.vertexModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_context.device, m_context.vertexModule, nullptr);
	if (m_context.fragmentModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_context.device, m_context.fragmentModule, nullptr);
	if (m_context.device != VK_NULL_HANDLE)
		vkDestroyDevice(m_context.device, nullptr);										// + cleanup of <VkQueue>
	if (m_context.surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);
	if (m_context.instance != VK_NULL_HANDLE)
		vkDestroyInstance(m_context.instance, nullptr);									// + cleanup of <VkPhysicalDevice>

	std::cout << "[VK] \tCleanup of handles --- DONE\n" << std::endl;
}

std::string pje::renderer::RendererVK::getApiVersion() {
	uint32_t apiVersion;
	vkEnumerateInstanceVersion(&apiVersion);
	uint32_t apiVersionMajor = VK_VERSION_MAJOR(apiVersion);
	uint32_t apiVersionMinor = VK_VERSION_MINOR(apiVersion);
	uint32_t apiVersionPatch = VK_VERSION_PATCH(apiVersion);

	return std::to_string(apiVersionMajor) + "." + std::to_string(apiVersionMinor) + "." + std::to_string(apiVersionPatch);
}

void pje::renderer::RendererVK::setInstance() {
	if (m_context.instance == VK_NULL_HANDLE) {
		VkApplicationInfo appInfo = {};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext				= nullptr;
		appInfo.pApplicationName	= "bachelor";
		appInfo.applicationVersion	= VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName			= "PJE";
		appInfo.engineVersion		= VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_3;

		std::vector<const char*> enabledLayers(m_context.instanceLayers.size());
		for (size_t i = 0; i < m_context.instanceLayers.size(); i++)
			enabledLayers[i] = m_context.instanceLayers[i].c_str();

		std::vector<const char*> enabledExtension(m_context.instanceExtensions.size());
		for (size_t i = 0; i < m_context.instanceExtensions.size(); i++)
			enabledExtension[i] = m_context.instanceExtensions[i].c_str();

		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo			= &appInfo;
		instanceInfo.pNext						= nullptr;
		instanceInfo.enabledLayerCount			= static_cast<uint32_t>(m_context.instanceLayers.size());
		instanceInfo.ppEnabledLayerNames		= enabledLayers.data();
		instanceInfo.enabledExtensionCount		= static_cast<uint32_t>(m_context.instanceExtensions.size());
		instanceInfo.ppEnabledExtensionNames	= enabledExtension.data();

		vkCreateInstance(&instanceInfo, nullptr, &m_context.instance);
		if (m_context.instance != VK_NULL_HANDLE)
			std::cout << "[VK] \tInstance was set." << std::endl;
		else
			throw std::runtime_error("Failed to set m_context.instance.");
	}
	else {
		throw std::runtime_error("Instance is already set!");
	}
}

void pje::renderer::RendererVK::setSurface(GLFWwindow* window) {
	VkResult res = glfwCreateWindowSurface(m_context.instance, window, nullptr, &m_context.surface);

	if (m_context.instance != VK_NULL_HANDLE && res == VK_SUCCESS)
		std::cout << "[VK] \tSurface was set." << std::endl;
	else {
		std::cout << "[VK] \tVkResult = " << res << std::endl;
		throw std::runtime_error("Failed to set VkSurfaceKHR.");
	}
}

void pje::renderer::RendererVK::setPhysicalDevice(VkPhysicalDeviceType gpuType,
												  VkQueueFlags requiredQueueAttributes,
												  uint32_t requiredSurfaceImages,
												  const std::vector<VkSurfaceFormatKHR>& formatOptions) {
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(m_context.instance, &gpuCount, nullptr);
	std::vector<VkPhysicalDevice> gpus(gpuCount);
	vkEnumeratePhysicalDevices(m_context.instance, &gpuCount, gpus.data());

	if (gpus.size() == 0)
		throw std::runtime_error("No GPU was found on this machine.");

	const uint8_t	AMOUNT_OF_REQUIREMENTS	= 5;
	uint8_t			requirementCounter		= 0;

	for (const auto& gpu : gpus) {
		requirementCounter = 0;

		/* Requirement 1) GPU type [ DISCRETE | INTEGRATED | ... ] */
		{
			VkPhysicalDeviceProperties gpuProps;
			vkGetPhysicalDeviceProperties(gpu, &gpuProps);

			std::cout << "[VK] \tGPU [" << gpuProps.deviceName << "] identified." << std::endl;
			if (gpuProps.deviceType == gpuType) {
				++requirementCounter;
				std::cout << "\t\t[OKAY] Required GPU type was found." << std::endl;
			}
			else {
				std::cout << "\t\t[ALERT] GPU does not suffice required gpu type." << std::endl;
				continue;
			}
		}

		/* Requirement 2) Queue family for [ GRAPHICS | COMPUTE | ... ] */
		{
			bool foundQueueFamily = false;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilyProps.data());

			for (uint32_t i = 0; i < queueFamilyCount; i++) {
				if ((requiredQueueAttributes & queueFamilyProps[i].queueFlags) == requiredQueueAttributes) {
					++requirementCounter;
					foundQueueFamily = true;
					std::cout << "\t\t[OKAY] GPU offers queue family with required attributes. (Family index: " << i << ")" << std::endl;
				}
			}

			if (!foundQueueFamily) {
				std::cout << "\t\t[ALERT] GPU does not possess a queue family with required attributes." << std::endl;
				continue;
			}

		}

		/* Requirement 3) Surface image count */
		{
			VkSurfaceCapabilitiesKHR surfaceCaps;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_context.surface, &surfaceCaps);

			if (surfaceCaps.maxImageCount >= requiredSurfaceImages) {
				++requirementCounter;
				std::cout << "\t\t[OKAY] GPU allows the required surface image amount." << std::endl;
			}
			else {
				std::cout << "\t\t[ALERT] GPU does not allow the required surface image amount." << std::endl;
				continue;
			}
		}

		/* Requirement 4) Surface Format for rendertargets, ... */
		{
			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_context.surface, &formatCount, nullptr);
			std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_context.surface, &formatCount, surfaceFormats.data());

			/* Double for-loop: check each available surfaceFormat if it suffices one of the given formatOptions and return the first valid entry */
			auto isValidLambda = [&formatOptions](const VkSurfaceFormatKHR& format) {
				return std::find_if(formatOptions.begin(), formatOptions.end(), [&format](const VkSurfaceFormatKHR& validFormat) {
					return format.format == validFormat.format && format.colorSpace == validFormat.colorSpace;
				}) != formatOptions.end();
			};

			auto it = std::find_if(surfaceFormats.begin(), surfaceFormats.end(), isValidLambda);
			if (it != surfaceFormats.end()) {
				std::cout << 
					"\t\t[OKAY] GPU offers at least one of the PJE valid surface formats: " << 
					it->format << " | " << it->colorSpace << 
				std::endl;

				setSurfaceFormatForPhysicalDevice(*it);
				++requirementCounter;
			}
			else {
				std::cout << "\t\t[ALERT] GPU does not provide one of the PJE valid surface formats." << std::endl;
				continue;
			}
		}

		/* Requirement 5) Presentation Mode [ VSync ] */
		{
			bool foundPresentMode = false;
			VkPresentModeKHR requiredMode = VkPresentModeKHR::VK_PRESENT_MODE_MAX_ENUM_KHR;
			if (m_vsync)
				requiredMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
			else
				requiredMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;

			uint32_t modeCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_context.surface, &modeCount, nullptr);
			std::vector<VkPresentModeKHR> presentModes(modeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_context.surface, &modeCount, presentModes.data());

			for (const auto& mode : presentModes) {
				if (mode == requiredMode) {
					foundPresentMode = true;
					break;
				}
			}

			if (!foundPresentMode) {
				std::cout << "\t\t[ALERT] GPU does not provide required presentation mode." << std::endl;
				continue;
			}
			else {
				std::cout << "\t\t[OKAY] GPU supports required presentation mode." << std::endl;
				++requirementCounter;
			}
		}

		if (requirementCounter == AMOUNT_OF_REQUIREMENTS) {
			m_context.gpu = gpu;
			std::cout << "\t\t[INFO] GPU selected." << std::endl;
			return;
		}
	}

	throw std::runtime_error("No suitable GPU was found for PJE.");
}

void pje::renderer::RendererVK::setSurfaceFormatForPhysicalDevice(VkSurfaceFormatKHR format) {
	m_context.surfaceFormat = format;
}

void pje::renderer::RendererVK::setDeviceAndQueue(VkQueueFlags requiredQueueAttributes) {
	/* 1) Gathering data for VkDeviceCreateInfo */
	bool foundQueueFamily = false;
	uint32_t selectedQueueFamily;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_context.gpu, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_context.gpu, &queueFamilyCount, queueFamilyProps.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if ((requiredQueueAttributes & queueFamilyProps[i].queueFlags) == requiredQueueAttributes) {
			foundQueueFamily = true;
			selectedQueueFamily = i;
			break;
		}
	}
	if (!foundQueueFamily)
		throw std::runtime_error("m_context.gpu does not possess a queue family with required attributes.");

	std::vector<const char*> enabledExtensions(m_context.deviceExtensions.size());
	for (size_t i = 0; i < m_context.deviceExtensions.size(); i++)
		enabledExtensions[i] = m_context.deviceExtensions[i].c_str();
	const float queuePrios[]{ 1.0f };

	VkBool32 isPresentationOnSurfaceAvailable;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_context.gpu, selectedQueueFamily, m_context.surface, &isPresentationOnSurfaceAvailable);
	if (!isPresentationOnSurfaceAvailable) {
		throw std::runtime_error("Selected queue family does not support presentation on given surface.");
	}

	/* 2) Defining queue and device info for their creation */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext				= nullptr;
	deviceQueueInfo.flags				= 0;
	deviceQueueInfo.queueFamilyIndex	= selectedQueueFamily;
	deviceQueueInfo.queueCount			= 1;
	deviceQueueInfo.pQueuePriorities	= queuePrios;

	VkDeviceCreateInfo deviceInfo;
	deviceInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.flags					= 0;
	deviceInfo.queueCreateInfoCount		= 1;
	deviceInfo.pQueueCreateInfos		= &deviceQueueInfo;
	deviceInfo.enabledLayerCount		= 0;
	deviceInfo.ppEnabledLayerNames		= nullptr;
	deviceInfo.enabledExtensionCount	= static_cast<uint32_t>(enabledExtensions.size());
	deviceInfo.ppEnabledExtensionNames	= enabledExtensions.data();
	deviceInfo.pEnabledFeatures			= nullptr;

	/* 3) Deciding on device features + creating device */
	if (m_anisotropyLevel != AnisotropyLevel::Disabled) {
		VkPhysicalDeviceFeatures	deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		VkPhysicalDeviceFeatures2	deviceFeatures2{ VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		deviceFeatures2.features = deviceFeatures;

		deviceInfo.pNext = &deviceFeatures2;
		vkCreateDevice(m_context.gpu, &deviceInfo, nullptr, &m_context.device);
	}
	else {
		deviceInfo.pNext = nullptr;
		vkCreateDevice(m_context.gpu, &deviceInfo, nullptr, &m_context.device);
	}

	/* 4) Checking if device was created */
	if (m_context.device != VK_NULL_HANDLE)
		std::cout << "[VK] \tDevice was set." << std::endl;
	else
		throw std::runtime_error("Failed to set device.");

	/* 5) Getting queue handle of created device */
	vkGetDeviceQueue(m_context.device, selectedQueueFamily, 0, &m_context.deviceQueue);
	if (m_context.deviceQueue != VK_NULL_HANDLE)
		std::cout << "[VK] \tDevice queue was set." << std::endl;
	else
		throw std::runtime_error("Failed to set device queue.");
}

void pje::renderer::RendererVK::setShaderModule(VkShaderModule& shaderModule, const std::vector<char>& shaderCode) {
	if (shaderModule != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_context.device, shaderModule, nullptr);

	VkShaderModuleCreateInfo shaderModuleInfo;
	shaderModuleInfo.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.pNext		= nullptr;
	shaderModuleInfo.flags		= 0;
	shaderModuleInfo.codeSize	= shaderCode.size();
	shaderModuleInfo.pCode		= (uint32_t*)shaderCode.data();

	vkCreateShaderModule(m_context.device, &shaderModuleInfo, nullptr, &shaderModule);
	if (shaderModule == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create shader module.");
}

void pje::renderer::RendererVK::setShaderProgram(std::string shaderName) {
	if (!m_context.shaderProgram.empty())
		m_context.shaderProgram.clear();

	auto vertexShader	= loadShader("assets/shaders/" + shaderName + ".vert.spv");
	auto fragmentShader = loadShader("assets/shaders/" + shaderName + ".frag.spv");

	setShaderModule(m_context.vertexModule, vertexShader);
	setShaderModule(m_context.fragmentModule, fragmentShader);

	VkPipelineShaderStageCreateInfo shaderStageInfo;
	shaderStageInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext				= nullptr;
	shaderStageInfo.flags				= 0;
	shaderStageInfo.pName				= "main";
	shaderStageInfo.pSpecializationInfo = nullptr;

	shaderStageInfo.stage	= VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfo.module	= m_context.vertexModule;
	m_context.shaderProgram.push_back(shaderStageInfo);

	shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfo.module = m_context.fragmentModule;
	m_context.shaderProgram.push_back(shaderStageInfo);

	std::cout << "[VK] \tShader program with " << m_context.shaderProgram.size() << " shaders was set." << std::endl;
}

void pje::renderer::RendererVK::setFence(VkFence& fence, bool isSignaled) {
	if (fence != VK_NULL_HANDLE)
		vkDestroyFence(m_context.device, fence, nullptr);

	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	if (isSignaled)
		info.flags = 1;
	else
		info.flags = 0;

	vkCreateFence(m_context.device, &info, nullptr, &fence);
	if (fence == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create fence.");
	std::cout << "[VK] \tA fence was set." << std::endl;
}

void pje::renderer::RendererVK::setSemaphore(VkSemaphore& sem) {
	if (sem != VK_NULL_HANDLE)
		vkDestroySemaphore(m_context.device, sem, nullptr);

	VkSemaphoreCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;

	vkCreateSemaphore(m_context.device, &info, nullptr, &sem);
	if (sem == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create semaphore.");
	std::cout << "[VK] \tA semaphore was set." << std::endl;
}

void pje::renderer::RendererVK::setDescriptorSet() {
	// TODO
}

void pje::renderer::RendererVK::setRenderpass() {
	/* Attachments: Describing rendertargets */
	VkAttachmentDescription attachmentMSAA;
	attachmentMSAA.flags			= 0;
	attachmentMSAA.format			= m_context.surfaceFormat.format;
	attachmentMSAA.samples			= m_msaaFactor;
	attachmentMSAA.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentMSAA.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentMSAA.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentMSAA.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentMSAA.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentMSAA.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachmentDepth;
	attachmentDepth.flags			= 0;
	attachmentDepth.format			= VkFormat::VK_FORMAT_D32_SFLOAT;
	attachmentDepth.samples			= m_msaaFactor;
	attachmentDepth.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDepth.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDepth.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDepth.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDepth.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDepth.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachmentResolve;
	attachmentResolve.flags				= 0;
	attachmentResolve.format			= m_context.surfaceFormat.format;
	attachmentResolve.samples			= VK_SAMPLE_COUNT_1_BIT;
	attachmentResolve.loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentResolve.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	attachmentResolve.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentResolve.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentResolve.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentResolve.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	/* References: Making attachments reusable for multiple subpasses */
	VkAttachmentReference attachmentRefMSAA;
	attachmentRefMSAA.attachment	= 0;
	attachmentRefMSAA.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachmentRefDepth;
	attachmentRefDepth.attachment	= 1;
	attachmentRefDepth.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachmentRefResolve;
	attachmentRefResolve.attachment = 2;
	attachmentRefResolve.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* 1 renderpass <-> 1+ subpasses */
	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentRefMSAA;
	subpassDescription.pDepthStencilAttachment = &attachmentRefDepth;
	subpassDescription.pResolveAttachments = &attachmentRefResolve;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;

	/* Dependencies: synchronization between subpasses */
	VkSubpassDependency depOrderedResolves;
	depOrderedResolves.srcSubpass		= VK_SUBPASS_EXTERNAL;
	depOrderedResolves.dstSubpass		= 0;
	depOrderedResolves.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depOrderedResolves.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depOrderedResolves.srcAccessMask	= VK_ACCESS_NONE;
	depOrderedResolves.dstAccessMask	= VK_ACCESS_NONE;
	depOrderedResolves.dependencyFlags	= 0;

	VkSubpassDependency depSharedDepthBuf;
	depSharedDepthBuf.srcSubpass		= VK_SUBPASS_EXTERNAL;
	depSharedDepthBuf.dstSubpass		= 0;
	depSharedDepthBuf.srcStageMask		= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depSharedDepthBuf.dstStageMask		= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depSharedDepthBuf.srcAccessMask		= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depSharedDepthBuf.dstAccessMask		= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	depSharedDepthBuf.dependencyFlags	= 0;

	VkAttachmentDescription attachmentDesc[]{ attachmentMSAA, attachmentDepth, attachmentResolve };
	VkSubpassDependency		subpassDep[]{ depOrderedResolves, depSharedDepthBuf };

	/* Renderpass creation */
	VkRenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext			= nullptr;
	renderPassInfo.flags			= 0;
	renderPassInfo.attachmentCount	= 3;
	renderPassInfo.pAttachments		= attachmentDesc;
	renderPassInfo.subpassCount		= 1;
	renderPassInfo.pSubpasses		= &subpassDescription;
	renderPassInfo.dependencyCount	= 2;
	renderPassInfo.pDependencies	= subpassDep;
	vkCreateRenderPass(m_context.device, &renderPassInfo, nullptr, &m_context.renderPass);

	if (m_context.renderPass == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create renderpass.");
	std::cout << "[VK] \tRenderpass was set." << std::endl;
}

void pje::renderer::RendererVK::buildPipeline() {
	VkViewport initViewport;
	initViewport.x			= 0.0f;
	initViewport.y			= 0.0f;
	initViewport.width		= m_renderWidth;
	initViewport.height		= m_renderHeight;
	initViewport.minDepth	= 0.0f;
	initViewport.maxDepth	= 1.0f;

	VkRect2D initScissor;
	initScissor.offset = { 0, 0 };
	initScissor.extent = { m_renderWidth, m_renderHeight };

	/* PipelineViewport: Viewport + Scissor */
	VkPipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pNext			= nullptr;
	viewportInfo.flags			= 0;
	viewportInfo.viewportCount	= 1;
	viewportInfo.pViewports		= &initViewport;
	viewportInfo.scissorCount	= 1;
	viewportInfo.pScissors		= &initScissor;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext						= nullptr;
	inputAssemblyInfo.flags						= 0;
	inputAssemblyInfo.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable	= VK_FALSE;

	auto vertexBindingDesc = pje::engine::types::Vertex::getVulkanBindingDesc();
	auto vertexAttribsDesc = pje::engine::types::Vertex::getVulkanAttribDesc();

	/* Holds vertex attributes */
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext							= nullptr;
	vertexInputInfo.flags							= 0;
	vertexInputInfo.vertexBindingDescriptionCount	= 1;
	vertexInputInfo.pVertexBindingDescriptions		= &vertexBindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = vertexAttribsDesc.size();
	vertexInputInfo.pVertexAttributeDescriptions	= vertexAttribsDesc.data();

	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	rasterizationInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.pNext						= nullptr;
	rasterizationInfo.flags						= 0;
	rasterizationInfo.depthClampEnable			= VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable	= VK_FALSE;
	rasterizationInfo.polygonMode				= VK_POLYGON_MODE_FILL;
	rasterizationInfo.cullMode					= VkCullModeFlagBits::VK_CULL_MODE_NONE;
	rasterizationInfo.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationInfo.depthBiasEnable			= VK_FALSE;
	rasterizationInfo.depthBiasConstantFactor	= 0.0f;
	rasterizationInfo.depthBiasClamp			= 0.0f;
	rasterizationInfo.depthBiasSlopeFactor		= 0.0f;
	rasterizationInfo.lineWidth					= 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.blendEnable			= VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor	= VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor	= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp			= VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp			= VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask			= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	colorBlendInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.pNext			= nullptr;
	colorBlendInfo.flags			= 0;
	colorBlendInfo.logicOpEnable	= VK_FALSE;
	colorBlendInfo.logicOp			= VK_LOGIC_OP_NO_OP;
	colorBlendInfo.attachmentCount	= 1;
	colorBlendInfo.pAttachments		= &colorBlendAttachment;
	std::fill_n(colorBlendInfo.blendConstants, 4, 0.0f);

	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	multisampleInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.pNext					= nullptr;
	multisampleInfo.flags					= 0;
	multisampleInfo.rasterizationSamples	= m_msaaFactor;
	multisampleInfo.sampleShadingEnable		= VK_FALSE;
	multisampleInfo.minSampleShading		= 1.0f;
	multisampleInfo.pSampleMask				= nullptr;
	multisampleInfo.alphaToCoverageEnable	= VK_FALSE;
	multisampleInfo.alphaToOneEnable		= VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
	depthStencilStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateInfo.pNext				= nullptr;
	depthStencilStateInfo.flags				= 0;
	depthStencilStateInfo.depthTestEnable	= VK_TRUE;
	depthStencilStateInfo.depthWriteEnable	= VK_TRUE;
	depthStencilStateInfo.depthCompareOp	= VK_COMPARE_OP_LESS;
	depthStencilStateInfo.minDepthBounds	= 0.0f;
	depthStencilStateInfo.maxDepthBounds	= 1.0f;

	/* Defining dynamic members for VkGraphicsPipelineCreateInfo */
	std::array<VkDynamicState, 2> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	dynamicStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.pNext				= nullptr;
	dynamicStateInfo.flags				= 0;
	dynamicStateInfo.dynamicStateCount	= dynamicStates.size();
	dynamicStateInfo.pDynamicStates		= dynamicStates.data();

	/* PipelineLayout: Declares variables of programmable shaders */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_context.descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	vkCreatePipelineLayout(m_context.device, &pipelineLayoutInfo, nullptr, &m_context.pipelineLayout);

	/* PipelineInfo: Defines workflow for a subpass of some renderpass */
	VkGraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext					= nullptr;
	pipelineInfo.flags					= 0;
	pipelineInfo.stageCount				= m_context.shaderProgram.size();
	pipelineInfo.pStages				= m_context.shaderProgram.data();
	pipelineInfo.pVertexInputState		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState	= &inputAssemblyInfo;
	pipelineInfo.pTessellationState		= nullptr;
	pipelineInfo.pViewportState			= &viewportInfo;
	pipelineInfo.pRasterizationState	= &rasterizationInfo;
	pipelineInfo.pMultisampleState		= &multisampleInfo;
	pipelineInfo.pDepthStencilState		= &depthStencilStateInfo;
	pipelineInfo.pColorBlendState		= &colorBlendInfo;
	pipelineInfo.pDynamicState			= &dynamicStateInfo;
	pipelineInfo.layout					= m_context.pipelineLayout;
	pipelineInfo.renderPass				= m_context.renderPass;
	pipelineInfo.subpass				= 0;
	pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex		= -1;
	vkCreateGraphicsPipelines(m_context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_context.pipeline);

	if (m_context.pipeline == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create pipeline.");
	std::cout << "[VK] \tPipeline was set." << std::endl;
}

bool pje::renderer::RendererVK::requestLayer(RequestLevel level, std::string layerName) {
	switch (level) {
	case pje::renderer::RendererVK::RequestLevel::Instance:
		{
			uint32_t propertyCount = 0;
			vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);
			std::vector<VkLayerProperties> availableLayers(propertyCount);
			vkEnumerateInstanceLayerProperties(&propertyCount, availableLayers.data());

			for (const auto& layer : availableLayers) {
				if (strcmp(layer.layerName, layerName.c_str()) == 0) {
					m_context.instanceLayers.push_back(layerName);
					std::cout << "[VK] \tRequested layer: " << layer.layerName << "\n\t\t[ADDED] ==> m_context.instance." << std::endl;
					return true;
				}
			}
		}
		return false;
	case pje::renderer::RendererVK::RequestLevel::Device:
		std::cout << "[VK] \tPJE doesn't support deprecated device layers." << std::endl;
		return false;
	default:
		return false;
	}
}

bool pje::renderer::RendererVK::requestExtension(RequestLevel level, std::string extensionName) {
	switch (level) {
	case pje::renderer::RendererVK::RequestLevel::Instance:
		{
			uint32_t propertyCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(propertyCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, availableExtensions.data());

			for (const auto& extension : availableExtensions) {
				if (strcmp(extension.extensionName, extensionName.c_str()) == 0) {
					m_context.instanceExtensions.push_back(extensionName);
					std::cout << "[VK] \tRequested extension: " << extension.extensionName << "\n\t\t[ADDED] ==> m_context.instance." << std::endl;
					return true;
				}
			}
		}
		return false;
	case pje::renderer::RendererVK::RequestLevel::Device:
		{
			if (m_context.gpu == VK_NULL_HANDLE)
				return false;

			uint32_t propertyCount = 0;
			vkEnumerateDeviceExtensionProperties(m_context.gpu, nullptr, &propertyCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(propertyCount);
			vkEnumerateDeviceExtensionProperties(m_context.gpu, nullptr, &propertyCount, availableExtensions.data());

			for (const auto& extension : availableExtensions) {
				if (strcmp(extension.extensionName, extensionName.c_str()) == 0) {
					m_context.deviceExtensions.push_back(extensionName);
					std::cout << "[VK] \tRequested extension: " << extension.extensionName << "\n\t\t[ADDED] ==> m_context.device." << std::endl;
					return true;
				}
			}
		}
		return false;
	default:
		return false;
	}
}

void pje::renderer::RendererVK::requestGlfwInstanceExtensions() {
	uint32_t	extensionCount;
	size_t		checkSum(0);

	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	for (uint32_t i = 0; i < extensionCount; i++) {
		std::cout << "[GLFW]\tRequiring Instance Extension: " << extensions[i] << " ..." << std::endl;
		if (requestExtension(RequestLevel::Instance, std::string(extensions[i])))
			++checkSum;
	}

	if (checkSum == extensionCount)
		std::cout << "[GLFW]\tAll required GLFW extensions were found." << std::endl;
	else
		throw std::runtime_error("Current machine isn't providing all required GLFW extensions.");
}

std::vector<char> pje::renderer::RendererVK::loadShader(const std::string& filename) {
	// using ios:ate FLAG to retrieve fileSize => ate sets pointer to the end (ate => at end) ; _Prot default is set to 64
	std::ifstream currentFile(filename, std::ios::binary | std::ios::ate);

	// ifstream converts object currentFile to TRUE if the file was opened successfully
	if (currentFile) {
		// size_t guarantees to hold any array index => here tellg and ios::ate helps to get the size of the file
		size_t currentFileSize = (size_t)currentFile.tellg();
		std::vector<char> buffer(currentFileSize);

		// sets reading head to the beginning of the file
		currentFile.seekg(0);
		// reads ENTIRE binary into RAM for the program
		currentFile.read(buffer.data(), currentFileSize);
		// close will be called by destructor at the end of the scope but this line helps to understand the code
		currentFile.close();

		return buffer;
	}
	else {
		throw std::runtime_error("Failed to load a shader into RAM!");
	}
}