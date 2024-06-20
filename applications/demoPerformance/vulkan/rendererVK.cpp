#include "rendererVK.h"
#define DEBUG

pje::renderer::RendererVK::RendererVK(const pje::engine::ArgsParser& parser, GLFWwindow* const window, pje::engine::types::LSysObject renderable) : 
	RendererInterface(), m_context(), m_renderWidth(parser.m_width), m_renderHeight(parser.m_height), 
	m_vsync(parser.m_vsync), m_anisotropyLevel(AnisotropyLevel::TWOx), m_msaaFactor(VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT) {

	std::cout << "[VK] \tVulkan Version: " << getApiVersion() << std::endl;

#ifdef DEBUG
	if (!requestLayer(RequestLevel::Instance, "VK_LAYER_KHRONOS_validation"))
		throw std::runtime_error("Validation layer for instance dispatch chain was not found.");
#endif // DEBUG
	requestGlfwInstanceExtensions();

	/* 1) Setup of basic Vulkan environment */
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

	/* 2) Declaration of shader code structure */
	setDescriptorSet(
		std::vector<DescriptorSetElementVK>{
			{VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT}, 
			{VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(renderable.m_boneRefs.size()), VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT}, 
			{VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(renderable.m_bones.size()), VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT}, 
			{VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT}
		}
	);
	setRenderpass();
	buildPipeline();

	/* 3) Rendertargets (results of graphical computations) */
	setRendertarget(
		m_context.rtMsaa,
		VkExtent3D{ m_renderWidth, m_renderHeight, 1 },
		m_context.surfaceFormat.format,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT
	);
	setRendertarget(
		m_context.rtDepth,
		VkExtent3D{ m_renderWidth, m_renderHeight, 1 },
		VkFormat::VK_FORMAT_D32_SFLOAT,
		VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT
	);
	setSwapchain(VkExtent2D{ m_renderWidth, m_renderHeight });

	/* 4) Framebuffers: bridge between rendertargets and renderpass */
	setFramebuffers(3);

	/* 5) Synchronization tools */
	setFence(m_context.fenceSetupTask, false);
	setFence(m_context.fenceImgRendered, true);
	setSemaphore(m_context.semSwapImgReceived);
	setSemaphore(m_context.semImgRendered);

	/* 6) Commandbuffers: sends commands to GPU */
	setCommandPool();
	//setCommandbuffer();
}

pje::renderer::RendererVK::~RendererVK() {
	/* Better solution: using vma */
	vkDeviceWaitIdle(m_context.device);
	std::cout << "[VK] \tCleaning up handles ..." << std::endl;

	if (m_context.commandPool != VK_NULL_HANDLE)
		vkDestroyCommandPool(m_context.device, m_context.commandPool, nullptr);			// + freeing <Commandbuffer>

	if (m_context.semImgRendered != VK_NULL_HANDLE)
		vkDestroySemaphore(m_context.device, m_context.semImgRendered, nullptr);
	if (m_context.semSwapImgReceived != VK_NULL_HANDLE)
		vkDestroySemaphore(m_context.device, m_context.semSwapImgReceived, nullptr);
	if (m_context.fenceImgRendered != VK_NULL_HANDLE)
		vkDestroyFence(m_context.device, m_context.fenceImgRendered, nullptr);
	if (m_context.fenceSetupTask != VK_NULL_HANDLE)
		vkDestroyFence(m_context.device, m_context.fenceSetupTask, nullptr);

	for (auto& fb : m_context.renderPassFramebuffers) {
		if (fb != VK_NULL_HANDLE)
			vkDestroyFramebuffer(m_context.device, fb, nullptr);
	}
	for (auto& imgView : m_context.swapchainImgViews) {
		if (imgView != VK_NULL_HANDLE)
			vkDestroyImageView(m_context.device, imgView, nullptr);
	}
	if (m_context.swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_context.device, m_context.swapchain, nullptr);

	if (m_context.rtDepth.imageView != VK_NULL_HANDLE)
		vkDestroyImageView(m_context.device, m_context.rtDepth.imageView, nullptr);
	if (m_context.rtDepth.memory != VK_NULL_HANDLE)
		vkFreeMemory(m_context.device, m_context.rtDepth.memory, nullptr);
	if (m_context.rtDepth.image != VK_NULL_HANDLE)
		vkDestroyImage(m_context.device, m_context.rtDepth.image, nullptr);
	if (m_context.rtMsaa.imageView != VK_NULL_HANDLE)
		vkDestroyImageView(m_context.device, m_context.rtMsaa.imageView, nullptr);
	if (m_context.rtMsaa.memory != VK_NULL_HANDLE)
		vkFreeMemory(m_context.device, m_context.rtMsaa.memory, nullptr);
	if (m_context.rtMsaa.image != VK_NULL_HANDLE)
		vkDestroyImage(m_context.device, m_context.rtMsaa.image, nullptr);

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

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_context.gpu, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_context.gpu, &queueFamilyCount, queueFamilyProps.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		if ((requiredQueueAttributes & queueFamilyProps[i].queueFlags) == requiredQueueAttributes) {
			foundQueueFamily = true;
			m_context.deviceQueueFamilyIndex = i;
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
	vkGetPhysicalDeviceSurfaceSupportKHR(m_context.gpu, m_context.deviceQueueFamilyIndex, m_context.surface, &isPresentationOnSurfaceAvailable);
	if (!isPresentationOnSurfaceAvailable) {
		throw std::runtime_error("Selected queue family does not support presentation on given surface.");
	}

	/* 2) Defining queue and device info for their creation */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext				= nullptr;
	deviceQueueInfo.flags				= 0;
	deviceQueueInfo.queueFamilyIndex	= m_context.deviceQueueFamilyIndex;
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
	vkGetDeviceQueue(m_context.device, m_context.deviceQueueFamilyIndex, 0, &m_context.deviceQueue);
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

void pje::renderer::RendererVK::setDescriptorSet(const std::vector<DescriptorSetElementVK>& elements) {
	if (m_context.descriptorPool != VK_NULL_HANDLE)
		throw std::runtime_error("Descriptor pool is already set!");
	if (m_context.descriptorSetLayout != VK_NULL_HANDLE)
		throw std::runtime_error("Descriptor set layout is already set!");

	/* 1) Descriptor Set Layout */
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	VkDescriptorSetLayoutBinding layoutBinding = {};
	uint32_t index = 0;

	layoutBinding.pImmutableSamplers	= nullptr;
	for (const auto& element : elements) {
		layoutBinding.binding			= index;
		layoutBinding.descriptorType	= element.type;
		layoutBinding.descriptorCount	= element.arraySize;
		layoutBinding.stageFlags		= element.flagsMask;

		layoutBindings.push_back(layoutBinding);
		++index;
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext			= nullptr;
	layoutCreateInfo.flags			= 0;
	layoutCreateInfo.bindingCount	= layoutBindings.size();
	layoutCreateInfo.pBindings		= layoutBindings.data();
	vkCreateDescriptorSetLayout(m_context.device, &layoutCreateInfo, nullptr, &m_context.descriptorSetLayout);

	if (m_context.descriptorSetLayout == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create descriptor set layout.");
	std::cout << "[VK] \tDescriptor set layout was set." << std::endl;

	/* 2) Descriptor Pool */
	std::vector<VkDescriptorPoolSize> poolElements;

	for (const auto& element : elements) {
		uint32_t amountOfThisType = 0;
		for (const auto& cmpElement : elements) {
			if (element.type == cmpElement.type)
				++amountOfThisType;
		}
		poolElements.push_back(VkDescriptorPoolSize{element.type, amountOfThisType});
	}

	VkDescriptorPoolCreateInfo poolCreateInfo;
	poolCreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.pNext			= nullptr;
	poolCreateInfo.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets			= 1;
	poolCreateInfo.poolSizeCount	= poolElements.size();
	poolCreateInfo.pPoolSizes		= poolElements.data();
	vkCreateDescriptorPool(m_context.device, &poolCreateInfo, nullptr, &m_context.descriptorPool);

	if (m_context.descriptorPool == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create descriptor pool.");
	std::cout << "[VK] \tDescriptor pool was set." << std::endl;

	/* 3) Descriptor Set */
	VkDescriptorSetAllocateInfo allocateInfo;
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.descriptorPool = m_context.descriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &m_context.descriptorSetLayout;
	vkAllocateDescriptorSets(m_context.device, &allocateInfo, &m_context.descriptorSet);

	if (m_context.descriptorSet == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to create descriptor set.");
	std::cout << "[VK] \tDescriptor set was set." << std::endl;
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
	subpassDescription.flags					= 0;
	subpassDescription.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount		= 1;
	subpassDescription.pColorAttachments		= &attachmentRefMSAA;
	subpassDescription.pDepthStencilAttachment	= &attachmentRefDepth;
	subpassDescription.pResolveAttachments		= &attachmentRefResolve;
	subpassDescription.preserveAttachmentCount	= 0;
	subpassDescription.pPreserveAttachments		= nullptr;
	subpassDescription.inputAttachmentCount		= 0;
	subpassDescription.pInputAttachments		= nullptr;

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

void pje::renderer::RendererVK::setRendertarget(ImageVK& rt, VkExtent3D imgSize, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask) {
	rt.format = format;
	
	if (rt.image != VK_NULL_HANDLE || rt.memory != VK_NULL_HANDLE || rt.imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_context.device, rt.imageView, nullptr);
		vkFreeMemory(m_context.device, rt.memory, nullptr);
		vkDestroyImage(m_context.device, rt.image, nullptr);
	}

	/* 1) Image */
	VkImageCreateInfo imageInfo;
	imageInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext					= nullptr;
	imageInfo.flags					= 0;
	imageInfo.imageType				= VkImageType::VK_IMAGE_TYPE_2D;
	imageInfo.format				= rt.format;
	imageInfo.extent				= imgSize;
	imageInfo.mipLevels				= 1;
	imageInfo.arrayLayers			= 1;
	imageInfo.samples				= m_msaaFactor;
	imageInfo.tiling				= VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage					= usage;
	imageInfo.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices	= nullptr;
	imageInfo.initialLayout			= VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(m_context.device, &imageInfo, nullptr, &rt.image);

	if (rt.image == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to set rendertarget's VkImage.");

	/* 2) Memory */
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(m_context.device, rt.image, &memReq);

	rt.memory = allocateMemory(memReq, VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkBindImageMemory(m_context.device, rt.image, rt.memory, 0);

	/* 3) Image View */
	VkImageViewCreateInfo viewInfo;
	viewInfo.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext				= nullptr;
	viewInfo.flags				= 0;
	viewInfo.image				= rt.image;
	viewInfo.viewType			= VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format				= rt.format;
	viewInfo.components			= VkComponentMapping{ 
		VK_COMPONENT_SWIZZLE_IDENTITY	// r,g,b,a stay as their identity
	};
	viewInfo.subresourceRange	= VkImageSubresourceRange{
		aspectMask,						// lets driver know what this is used for
		0,								// for mipmap
		1,								// -||-
		0,								// for specifying a layer in VkImage
		1								// -||-
	};
	vkCreateImageView(m_context.device, &viewInfo, nullptr, &rt.imageView);

	if (rt.imageView == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to set rendertarget's VkImageView.");
	std::cout << "[VK] \tA rendertarget was set." << std::endl;
}

void pje::renderer::RendererVK::setSwapchain(VkExtent2D imgSize) {
	/* 1) General Swapchain (VkImage + VkMemory managed by VkSwapchainKHR) */
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext					= nullptr;
	swapchainInfo.flags					= 0;
	swapchainInfo.surface				= m_context.surface;
	swapchainInfo.minImageCount			= m_context.surfaceImageCount;
	swapchainInfo.imageFormat			= m_context.surfaceFormat.format;
	swapchainInfo.imageColorSpace		= m_context.surfaceFormat.colorSpace;
	swapchainInfo.imageExtent			= imgSize;
	swapchainInfo.imageArrayLayers		= 1;
	swapchainInfo.imageUsage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices	= nullptr;
	swapchainInfo.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainInfo.compositeAlpha		= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.clipped				= VK_TRUE;
	swapchainInfo.oldSwapchain			= VK_NULL_HANDLE;
	if (m_vsync)
		swapchainInfo.presentMode		= VK_PRESENT_MODE_FIFO_KHR;
	else
		swapchainInfo.presentMode		= VK_PRESENT_MODE_IMMEDIATE_KHR;
	vkCreateSwapchainKHR(m_context.device, &swapchainInfo, nullptr, &m_context.swapchain);

	if (m_context.swapchain == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to set a swapchain.");

	/* 2) Getting VkImage and VkImageView */
	auto swapchainImages = std::vector<VkImage>(m_context.surfaceImageCount);
	vkGetSwapchainImagesKHR(m_context.device, m_context.swapchain, &m_context.surfaceImageCount, swapchainImages.data());

	VkImageViewCreateInfo viewInfo;
	viewInfo.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext				= nullptr;
	viewInfo.flags				= 0;
	viewInfo.viewType			= VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format				= m_context.surfaceFormat.format;
	viewInfo.components			= VkComponentMapping{ 
		VK_COMPONENT_SWIZZLE_IDENTITY 
	};
	viewInfo.subresourceRange	= VkImageSubresourceRange{
		VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
	};

	m_context.swapchainImgViews = std::vector<VkImageView>(m_context.surfaceImageCount);
	for (uint32_t i = 0; i < m_context.surfaceImageCount; i++) {
		viewInfo.image = swapchainImages[i];
		vkCreateImageView(m_context.device, &viewInfo, nullptr, &m_context.swapchainImgViews[i]);
		if (m_context.swapchainImgViews[i] == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to create image view for swapchain.");
	}
	std::cout << "[VK] \tSwapchain was set." << std::endl;
}

void pje::renderer::RendererVK::setFramebuffers(uint32_t renderPassAttachmentCount) {
	m_context.renderPassFramebuffers = std::vector<VkFramebuffer>(m_context.surfaceImageCount);
	VkFramebufferCreateInfo fbInfo;
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.pNext = nullptr;
	fbInfo.flags = 0;
	fbInfo.renderPass = m_context.renderPass;
	fbInfo.width = m_renderWidth;
	fbInfo.height = m_renderHeight;
	fbInfo.layers = 1;

	fbInfo.attachmentCount = renderPassAttachmentCount;
	for (uint32_t i = 0; i < m_context.surfaceImageCount; i++) {
		std::vector<VkImageView> targets{
			m_context.rtMsaa.imageView,
			m_context.rtDepth.imageView,
			m_context.swapchainImgViews[i]
		};
		fbInfo.pAttachments = targets.data();
		vkCreateFramebuffer(m_context.device, &fbInfo, nullptr, &m_context.renderPassFramebuffers[i]);
		if (m_context.renderPassFramebuffers[i] == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to create framebuffer.");
	}
	std::cout << "[VK] \tFramebuffers were set." << std::endl;
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

void pje::renderer::RendererVK::setCommandPool() {
	VkCommandPoolCreateInfo poolInfo;
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_context.deviceQueueFamilyIndex;
	vkCreateCommandPool(m_context.device, &poolInfo, nullptr, &m_context.commandPool);
	
	if (m_context.commandPool == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to set command pool.");
	std::cout << "[VK] \tCommand pool was set." << std::endl;
}

void pje::renderer::RendererVK::setCommandbuffer() {
	// TODO

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

	/* Color Blending => turned off */
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
	pipelineInfo.pDynamicState			= nullptr;
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

VkDeviceMemory pje::renderer::RendererVK::allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags flagMask) {
	/* 1) Searching memory type index of choosen GPU */
	bool foundIndex				= false;
	uint32_t memoryTypeIndex	= 0;

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(m_context.gpu, &memProps);

	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReq.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & flagMask) == flagMask) {
			foundIndex		= true;
			memoryTypeIndex = i;
			break;
		}
	}

	if (!foundIndex)
		throw std::runtime_error("No valid memory type was found while allocating memory for some GPU ressource.");

	/* 2) Allocating memory */
	VkDeviceMemory memory = VK_NULL_HANDLE;

	VkMemoryAllocateInfo allocInfo;
	allocInfo.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext				= nullptr;
	allocInfo.allocationSize	= memReq.size;
	allocInfo.memoryTypeIndex	= memoryTypeIndex;
	vkAllocateMemory(m_context.device, &allocInfo, nullptr, &memory);

	return memory;
}