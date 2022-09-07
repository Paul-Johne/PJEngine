#include <app.h>

namespace PJEngine {
	VkResult	result;

	VkInstance		vulkanInstance;
	VkSurfaceKHR	surface;
	VkDevice		logicalDevice;

	VkQueue queueForPrototyping;

	VkSwapchainKHR	swapchain;
	uint32_t		numberOfImagesInSwapchain = 0;
	VkImageView*	imageViews;
	VkFramebuffer*	framebuffers;

	VkShaderModule	shaderModuleBasicVert;
	VkShaderModule	shaderModuleBasicFrag;
	std::vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;

	VkPipelineLayout	pipelineLayout;
	VkRenderPass		renderPass;
	VkPipeline			pipeline;

	VkCommandPool		commandPool;
	VkCommandBuffer*	commandBuffers;

	const VkFormat		outputFormat = VK_FORMAT_B8G8R8A8_UNORM;				// TODO (availability must be checked)
	const VkClearValue	clearValueDefault = { 0.0f, 0.0f, 1.0f, 0.5f };			// DEFAULT BACKGROUND (RGB) for all rendered images

	VkSemaphore semaphoreSwapchainImageReceived;
	VkSemaphore semaphoreRenderingFinished;
	VkFence		fenceRenderFinished;											// 2 States: signaled, unsignaled

	GLFWwindow*		window;
	const uint32_t	WINDOW_WIDTH = 1280;
	const uint32_t	WINDOW_HEIGHT = 720;
}

using namespace std;

void printPhysicalDeviceStats(VkPhysicalDevice& device) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	uint32_t apiVersion = properties.apiVersion;

	cout << "Available GPU:\t\t" << properties.deviceName << endl;
	cout << "\t\t\t\tVulkan API Version:\t" << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << endl;
	cout << "\t\t\t\tDevice Type:\t\t" << properties.deviceType << endl;
	cout << "\t\t\t\tdiscreteQueuePriorities:\t" << properties.limits.discreteQueuePriorities << endl;

	/* FEATURES => what kind of shaders can be processed */
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	cout << "\t\t\t\tFeatures:" << endl;
	cout << "\t\t\t\t\tGeometry Shader:\t" << features.geometryShader << endl;

	/* memoryProperties => heaps/memory with flags */
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
	cout << "\t\t\t\tMemory Properties:" << endl;
	cout << "\t\t\t\t\tHeaps:\t" << memoryProperties.memoryHeaps->size << endl;

	/* GPUs have queues to solve tasks ; their respective attributes are clustered in families */
	uint32_t numberOfQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, nullptr);

	auto familyProperties = vector<VkQueueFamilyProperties>(numberOfQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, familyProperties.data());
	
	/* displays attributes of each queue family and the amount of available queues */
	cout << "\t\t\t\tNumber of Queue Families:\t" << numberOfQueueFamilies << endl;
	for (int i = 0; i < numberOfQueueFamilies; i++) {
		cout << "\t\t\t\tQueue Family #" << i << endl;
		cout << "\t\t\t\t\tQueues in Family:\t\t" << familyProperties[i].queueCount << endl;
		/* BITWISE AND CHECK */
		cout << "\t\t\t\t\tVK_QUEUE_GRAPHICS_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_COMPUTE_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_TRANSFER_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_SPARSE_BINDING_BIT\t" << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << endl;
	}

	/* SurfaceCapabilities => checks access to triple buffering etc. */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, PJEngine::surface, &surfaceCapabilities);
	cout << "\t\t\t\tSurface Capabilities:" << endl;
	cout << "\t\t\t\t\tminImageCount:\t\t" << surfaceCapabilities.minImageCount << endl;
	cout << "\t\t\t\t\tmaxImageCount:\t\t" << surfaceCapabilities.maxImageCount << endl;
	cout << "\t\t\t\t\tcurrentExtent:\t\t" \
		 << surfaceCapabilities.currentExtent.width << "x" << surfaceCapabilities.currentExtent.height << endl;

	/* SurfaceFormats => defines how colors are stored */
	uint32_t numberOfFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, PJEngine::surface, &numberOfFormats, nullptr);
	auto surfaceFormats = vector<VkSurfaceFormatKHR>(numberOfFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, PJEngine::surface, &numberOfFormats, surfaceFormats.data());

	cout << "\t\t\t\tVkFormats: " << endl;
	for (int i = 0; i < numberOfFormats; i++) {
		cout << "\t\t\t\t\tIndex:\t\t" << surfaceFormats[i].format << endl;
	}

	/* PresentationMode => how CPU and GPU may interact with swapchain images */
	uint32_t numberOfPresentationModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, PJEngine::surface, &numberOfPresentationModes, nullptr);
	auto presentModes = vector<VkPresentModeKHR>(numberOfPresentationModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, PJEngine::surface, &numberOfPresentationModes, presentModes.data());

	/* Index 0 is necessary for immediate presentation */
	cout << "\t\t\t\tPresentation Modes:" << endl;
	for (int i = 0; i < numberOfPresentationModes; i++) {
		cout << "\t\t\t\t\tIndex:\t\t" << presentModes[i] << endl;
	}
}

int startGlfw3(const char* windowName) {
	glfwInit();

	if (glfwVulkanSupported() == GLFW_FALSE) {
		cout << "Error at glfwVulkanSupported" << endl;
		return -1;
	}

	/* Settings for upcoming window creation */
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	PJEngine::window = glfwCreateWindow(PJEngine::WINDOW_WIDTH, PJEngine::WINDOW_HEIGHT, windowName, nullptr, nullptr);

	return 0;
}

void stopGlfw3() {
	glfwDestroyWindow(PJEngine::window);
	glfwTerminate();
}

vector<char> readSpirvFile(const string& filename) {
	// using ios:ate FLAG to retrieve fileSize ; _Prot default is set to 64
	ifstream currentFile(filename, ios::binary | ios::ate);

	// ifstream converts object currentFile to TRUE if the file was opened successfully
	if (currentFile) {
		// size_t guarantees to hold any array index => here tellg and ios::ate helps to get the size of the file
		size_t currentFileSize = (size_t) currentFile.tellg();
		vector<char> buffer(currentFileSize);

		// sets reading head to the beginning of the file
		currentFile.seekg(0);
		// reads ENTIRE binary into RAM for the program
		currentFile.read(buffer.data(), currentFileSize);
		// close will be called by destructor at the end of the scope but this line helps to understand the code
		currentFile.close();
		
		return buffer;
	} else {
		throw runtime_error("Failed to read shaderfile into RAM!");
	}
}

/* Transforming shader code of type vector<char> into VkShaderModule */
void createShaderModule(const vector<char>& spvCode, VkShaderModule& shaderModule) {
	VkShaderModuleCreateInfo shaderModuleInfo;
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.pNext = nullptr;
	shaderModuleInfo.flags = 0;
	shaderModuleInfo.codeSize = spvCode.size();
	shaderModuleInfo.pCode = (uint32_t*) spvCode.data();	// spv instructions always have a size of 32 bits, so casting is possible

	PJEngine::result = vkCreateShaderModule(PJEngine::logicalDevice, &shaderModuleInfo, nullptr, &shaderModule);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateShaderModule" << endl;
		throw runtime_error("Failed to create VkShaderModule");
	}
}

/* Adds element (module) to PJEngine::shaderStageInfos */
void addShaderModuleToShaderStages(VkShaderModule newModule, VkShaderStageFlagBits stageType, const char* shaderEntryPoint = "main") {
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo;

	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.pNext = nullptr;
	shaderStageCreateInfo.flags = 0;
	shaderStageCreateInfo.stage = stageType;
	shaderStageCreateInfo.module = newModule;
	shaderStageCreateInfo.pName = shaderEntryPoint;			// entry point of the module
	shaderStageCreateInfo.pSpecializationInfo = nullptr;	// declaring const variables helps Vulkan to optimise shader code

	PJEngine::shaderStageInfos.push_back(shaderStageCreateInfo);
}

void clearShaderStages() {
	PJEngine::shaderStageInfos.clear();
}

int startVulkan() {
	/* appInfo used for instanceInfo */
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;				// sType for driver
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Procedural Generation with Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PJ Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	cout << "Engine started.." << endl;

	/* LAYERS are additional features between Vulkan functions	*/
	/* which layers are available depends on the GPU			*/
	uint32_t numberOfLayers = 0;
	vkEnumerateInstanceLayerProperties(&numberOfLayers, nullptr);

	auto availableLayers = vector<VkLayerProperties>(numberOfLayers);
	vkEnumerateInstanceLayerProperties(&numberOfLayers, availableLayers.data());

	cout << "\n[GPU] Available Instance Layers:\t" << numberOfLayers << endl;
	for (int i = 0; i < numberOfLayers; i++) {
		cout << "\tLayer:\t" << availableLayers[i].layerName << endl;
		cout << "\t\t\t" << availableLayers[i].description << endl;
	}

	/* EXTENSIONS add functions like actually displaying rendered images	*/
	/* extensions can be part of a layer									*/
	uint32_t numberOfExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, nullptr);

	auto availableExtensions = vector<VkExtensionProperties>(numberOfExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, availableExtensions.data());

	cout << "\n[GPU] Available Instance Extensions:\t" << numberOfExtensions << endl;
	for (int i = 0; i < numberOfExtensions; i++) {
		cout << "\tExtension:\t" << availableExtensions[i].extensionName << endl;
	}

	/* GET ALL GLFW INSTANCE EXTENSIONS for current OS */
	uint32_t numberOfRequiredGlfwExtensions = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numberOfRequiredGlfwExtensions);

	cout << "\n[GLFW] REQUIRED Instance Extensions:\t" << numberOfRequiredGlfwExtensions << endl;
	for (int i = 0; i < numberOfRequiredGlfwExtensions; i++) {
		cout << "\tExtension:\t" << glfwExtensions[i] << endl;
	}

	// TODO ( does this layer always exist? )
	vector<const char*> usedInstanceLayers {
		"VK_LAYER_KHRONOS_validation"
	};

	/* instanceInfo used for the actual Vulkan instance */
	VkInstanceCreateInfo instanceInfo;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nullptr;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = usedInstanceLayers.size();
	instanceInfo.ppEnabledLayerNames = usedInstanceLayers.data();
	instanceInfo.enabledExtensionCount = numberOfRequiredGlfwExtensions;
	instanceInfo.ppEnabledExtensionNames = glfwExtensions;

	/* returns an enum for errorhandling ( e.g. VK_ERROR_OUT_OF DEVICE_MEMORY ) */
	PJEngine::result = vkCreateInstance(&instanceInfo, nullptr, &PJEngine::vulkanInstance);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateInstance" << endl;
		return PJEngine::result;
	}
		
	/* VkSurfaceKHR => creates surface based on an existing GLFW Window */
	/*  */
	PJEngine::result = glfwCreateWindowSurface(PJEngine::vulkanInstance, PJEngine::window, nullptr, &PJEngine::surface);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at glfwCreateWindowSurface" << endl;
		return PJEngine::result;
	}

	/* Vulkan differs between PHYSICAL and LOGICAL GPU reference */
	/* A physical reference is necessary for a logical reference */
	uint32_t numberOfPhysicalDevices = 0;

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter):
		1) get number of GPUs and stores value in 2nd parameter */
	vkEnumeratePhysicalDevices(PJEngine::vulkanInstance, &numberOfPhysicalDevices, nullptr);

	auto physicalDevices = vector<VkPhysicalDevice>(numberOfPhysicalDevices);

	/*	vkEnumeratePhysicalDevices has two functionalities:
		2) get reference for a number of GPUs and store them in an array of physical devices */
	vkEnumeratePhysicalDevices(PJEngine::vulkanInstance, &numberOfPhysicalDevices, physicalDevices.data());

	cout << "\nNumber of GPUs:\t\t" << numberOfPhysicalDevices << endl;

	/* Shows some properties and features for each available GPU */
	for (int i = 0; i < numberOfPhysicalDevices; i++) {
		printPhysicalDeviceStats(physicalDevices[i]);
	}

	array<float, 1> queuePriorities {
		1.0f
	};

	/* deviceQueueCreateInfo used for deviceInfo */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext = nullptr;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = 0;										// TODO ( analyse available families before setting one )
	deviceQueueInfo.queueCount = 1;												// TODO ( analyse available queues in family first )
	deviceQueueInfo.pQueuePriorities = queuePriorities.data();

	VkPhysicalDeviceFeatures usedFeatures = {};									// all possible features set to false

	/* EXTENSIONS on device level */
	array<const char*, 1> deviceExtensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME											// enables Swapchains for real time rendering
	};

	/* deviceInfo declares what resources will be claimed */
	/* deviceInfo is necessary for a logical reference    */
	VkDeviceCreateInfo deviceInfo;
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = nullptr;
	deviceInfo.flags = 0;
	deviceInfo.queueCreateInfoCount = 1;										// number of VkDeviceQueueCreateInfo (1+ possible)
	deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;
	deviceInfo.enabledExtensionCount = deviceExtensions.size();
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceInfo.pEnabledFeatures = &usedFeatures;

	/* LOGICAL GPU reference => ! choose the best GPU for your task ! */
	PJEngine::result = vkCreateDevice(physicalDevices[0], &deviceInfo, nullptr, &PJEngine::logicalDevice);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateDevice" << endl;
		return PJEngine::result;
	}

	/* Getting Queue of some logical device to assign tasks (CommandBuffer) later */
	vkGetDeviceQueue(PJEngine::logicalDevice, 0, 0, &PJEngine::queueForPrototyping);

	/* Checking whether Swapchains are usable or not on physical device */
	VkBool32 surfaceSupportsSwapchain = false;
	PJEngine::result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], 0, PJEngine::surface, &surfaceSupportsSwapchain);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkGetPhysicalDeviceSurfaceSupportKHR" << endl;
		return PJEngine::result;
	}
	if (!surfaceSupportsSwapchain) {
		cout << "Surface not support for the choosen GPU!" << endl;
		return surfaceSupportsSwapchain;
	}

	/* swapchainInfo for building a Swapchain */
	VkSwapchainCreateInfoKHR swapchainInfo;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.flags = 0;
	swapchainInfo.surface = PJEngine::surface;
	swapchainInfo.minImageCount = 2;												// requires AT LEAST 2 (double buffering)
	swapchainInfo.imageFormat = PJEngine::outputFormat;								// TODO ( choose dynamically )
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;				// TODO ( choose dynamically )
	swapchainInfo.imageExtent = VkExtent2D { PJEngine::WINDOW_WIDTH, PJEngine::WINDOW_HEIGHT };
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;						// image shared between multiple queues?
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;				// no additional transformations
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// no see through images
	swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;							// TODO ( choose dynamically )
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;									// resizing image needs new swapchain

	/* Setting Swapchain with swapchainInfo to logicalDevice */
	PJEngine::result = vkCreateSwapchainKHR(PJEngine::logicalDevice, &swapchainInfo, nullptr, &PJEngine::swapchain);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateSwapchainKHR" << endl;
		return PJEngine::result;
	}

	/* Gets images of Swapchain => should be already declared via swapchainInfo.minImageCount */
	PJEngine::numberOfImagesInSwapchain = 0;
	vkGetSwapchainImagesKHR(PJEngine::logicalDevice, PJEngine::swapchain, &PJEngine::numberOfImagesInSwapchain, nullptr);
	auto swapchainImages = vector<VkImage>(PJEngine::numberOfImagesInSwapchain);

	/* swapchainImages will hold the reference to VkImage(s), BUT to access them there has to be VkImageView(s) */
	PJEngine::result = vkGetSwapchainImagesKHR(PJEngine::logicalDevice, PJEngine::swapchain, &PJEngine::numberOfImagesInSwapchain, swapchainImages.data());
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkGetSwapchainImagesKHR" << endl;
		return PJEngine::result;
	}

	/* ImageViewInfo for building ImageView ! TODO ! */
	VkImageViewCreateInfo imageViewInfo;
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = PJEngine::outputFormat;			// TODO ( choose dynamically )
	imageViewInfo.components = VkComponentMapping {
		VK_COMPONENT_SWIZZLE_IDENTITY	// properties r,g,b,a stay as their identity 
	};
	imageViewInfo.subresourceRange = VkImageSubresourceRange {
		VK_IMAGE_ASPECT_COLOR_BIT,		// aspectMask : describes what kind of data is stored in image
		0,								// baseMipLevel
		1,								// levelCount : describes how many MipMap levels exist
		0,								// baseArrayLayer : for VR
		1								// layerCount : for VR
	};

	/* ImageView gives access to images in swapchainImages */
	PJEngine::imageViews = new VkImageView[PJEngine::numberOfImagesInSwapchain];
	for (int i = 0; i < PJEngine::numberOfImagesInSwapchain; i++) {
		imageViewInfo.image = swapchainImages[i];

		PJEngine::result = vkCreateImageView(PJEngine::logicalDevice, &imageViewInfo, nullptr, &PJEngine::imageViews[i]);
		if (PJEngine::result != VK_SUCCESS) {
			cout << "Error at vkCreateImageView" << endl;
			return PJEngine::result;
		}
	}

	/* Reading compiled shader code into RAM */
	auto shaderBasicVert = readSpirvFile("assets/shaders/basic.vert.spv");
	auto shaderBasicFrag = readSpirvFile("assets/shaders/basic.frag.spv");
	cout << "\n[DEBUG] Basic Shader Sizes: " << shaderBasicVert.size() << " Vert | Frag " << shaderBasicFrag.size() << endl;

	/* Transforming shader code of type vector<char> into VkShaderModule */
	createShaderModule(shaderBasicVert, PJEngine::shaderModuleBasicVert);
	createShaderModule(shaderBasicFrag, PJEngine::shaderModuleBasicFrag);

	/* Ensures that PJEngine::shaderStageInfos are empty before filling with data */
	if (!PJEngine::shaderStageInfos.empty())
		clearShaderStages();

	/* Wraping Shader Modules in a distinct CreateInfo for the Shaderpipeline and adding them to engine's stageInfos */
	addShaderModuleToShaderStages(PJEngine::shaderModuleBasicVert, VK_SHADER_STAGE_VERTEX_BIT);
	addShaderModuleToShaderStages(PJEngine::shaderModuleBasicFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
	cout << "\n[DEBUG] Programable Pipeline Stages in PJEngine::shaderStageInfos: " << PJEngine::shaderStageInfos.size() << endl;

	/* Viewport */
	VkViewport viewport;
	viewport.x = 0.0f;							// upper left corner
	viewport.y = 0.0f;							// upper left corner
	viewport.width = PJEngine::WINDOW_WIDTH;
	viewport.height = PJEngine::WINDOW_HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/* Scissor defines what part of the viewport will be used */
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { PJEngine::WINDOW_WIDTH,  PJEngine::WINDOW_HEIGHT };

	/* PIPELINE BUILDING */
	/* Shader pipelines consist out of fixed functions and programmable functions (shaders) */

	/* viewportInfo => combines viewport and scissor for the pipeline */
	VkPipelineViewportStateCreateInfo viewportInfo;
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pNext = nullptr;
	viewportInfo.flags = 0;
	viewportInfo.viewportCount = 1;							// number of viewport
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;							// number of scissors
	viewportInfo.pScissors = &scissor;

	/* Fixed Function : Input Assembler */
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// how single vertices will be assembled
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	/* Usally sizes of vertices and their attributes are defined here
	 * It has similar tasks to OpenGL's glBufferData(), glVertexAttribPointer() etc. */
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

	PJEngine::result = vkCreatePipelineLayout(PJEngine::logicalDevice, &pipelineLayoutInfo, nullptr, &PJEngine::pipelineLayout);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreatePipelineLayout" << endl;
		return PJEngine::result;
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

	/* ATTACHMENTs are input/output buffer to communicate with Vulkan */

	// Attachments need a description
	VkAttachmentDescription attachmentDescription;
	attachmentDescription.flags = 0;
	attachmentDescription.format = PJEngine::outputFormat;						// has the same format as the swapchainInfo.imageFormat
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;						// for multisampling
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// what happens with the buffer after loading data
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// what happens with the buffer after storing data
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// for stencilBuffer (discarding certain pixels)
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// for stencilBuffer (discarding certain pixels)
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// buffer as input
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// buffer as output

	// attachmentDescription is reusable for n attachments via VkAttachmentReference
	VkAttachmentReference attachmentRef;
	attachmentRef.attachment = 0;										// index of attachment description
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// CONVERTING: desc.initialLayout -> ref.layout -> desc.finalLayout

	/* each render pass holds 1+ sub render passes */

	// one subpass may hold references to multiple VkAttachmentReference(s)
	VkSubpassDescription subpassDescription;
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// what kind of computation will be done
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;							// actual vertices that being used for shader computation
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentRef;
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

	/* renderPass represents a single execution of the rendering pipeline */
	VkRenderPassCreateInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDescription;					// for subpassDescription's VkAttachmentReference(s)
	renderPassInfo.subpassCount = 1;										// WARNING (+2 subpasses) => transforming of initialLayout and layout
	renderPassInfo.pSubpasses = &subpassDescription;						// "all" subpasses of the current render pass
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

	/* Creating RenderPass for VkGraphicsPipelineCreateInfo */
	PJEngine::result = vkCreateRenderPass(PJEngine::logicalDevice, &renderPassInfo, nullptr, &PJEngine::renderPass);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateRenderPass" << endl;
		return PJEngine::result;
	}

	/* CreateInfo for the actual pipeline */
	VkGraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;								// number of programmable stages
	pipelineInfo.pStages = PJEngine::shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizationInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = nullptr;						// parts being changeable without building new pipeline
	pipelineInfo.layout = PJEngine::pipelineLayout;
	pipelineInfo.renderPass = PJEngine::renderPass;
	pipelineInfo.subpass = 0;									// index of .subpass
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;			// deriving from other pipeline to reduce loading time
	pipelineInfo.basePipelineIndex = -1;						// good coding style => invalid index

	PJEngine::result = vkCreateGraphicsPipelines(PJEngine::logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &PJEngine::pipeline);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateGraphicsPipelines" << endl;
		return PJEngine::result;
	}

	/* Framebuffer => connects imageView to attachment (Vulkan communication buffer) */
	PJEngine::framebuffers = new VkFramebuffer[PJEngine::numberOfImagesInSwapchain];
	for (int i = 0; i < PJEngine::numberOfImagesInSwapchain; i++) {
		VkFramebufferCreateInfo framebufferInfo;
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = PJEngine::renderPass;
		framebufferInfo.attachmentCount = 1;							// one framebuffer may store multiple imageViews
		framebufferInfo.pAttachments = &(PJEngine::imageViews[i]);		// imageView => reference to image in swapchain
		framebufferInfo.width = PJEngine::WINDOW_WIDTH;					// dimension
		framebufferInfo.height = PJEngine::WINDOW_HEIGHT;				// dimension
		framebufferInfo.layers = 1;										// dimension

		PJEngine::result = vkCreateFramebuffer(PJEngine::logicalDevice, &framebufferInfo, nullptr, &PJEngine::framebuffers[i]);
		if (PJEngine::result != VK_SUCCESS) {
			cout << "Error at vkCreateFramebuffer" << endl;
			return PJEngine::result;
		}
	}

	/* VkCommandPool holds CommandBuffer which are necessary to tell Vulkan what to do */
	VkCommandPoolCreateInfo commandPoolInfo;
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = 0;						// reset record of single frame buffer | set to buffer to 'transient' for optimisation
	commandPoolInfo.queueFamilyIndex = 0;			// must be the same as in VkDeviceQueueCreateInfo of the logical device & Queue Family 'VK_QUEUE_GRAPHICS_BIT' must be 1

	PJEngine::result = vkCreateCommandPool(PJEngine::logicalDevice, &commandPoolInfo, nullptr, &PJEngine::commandPool);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateCommandPool" << endl;
		return PJEngine::result;
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = PJEngine::commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// primary => processed directly | secondary => invoked by primary
	commandBufferAllocateInfo.commandBufferCount = PJEngine::numberOfImagesInSwapchain;		// each buffer in swapchain needs dedicated command buffer

	PJEngine::commandBuffers = new VkCommandBuffer[PJEngine::numberOfImagesInSwapchain];
	PJEngine::result = vkAllocateCommandBuffers(PJEngine::logicalDevice, &commandBufferAllocateInfo, PJEngine::commandBuffers);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkAllocateCommandBuffers" << endl;
		return PJEngine::result;
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
	renderPassBeginInfo.renderPass = PJEngine::renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { PJEngine::WINDOW_WIDTH, PJEngine::WINDOW_HEIGHT };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &PJEngine::clearValueDefault;

	/* RECORDING of CommandBuffers */
	for (int i = 0; i < PJEngine::numberOfImagesInSwapchain; i++) {
		PJEngine::result = vkBeginCommandBuffer(PJEngine::commandBuffers[i], &commandBufferBeginInfo);
		if (PJEngine::result != VK_SUCCESS) {
			cout << "Error at vkBeginCommandBuffer of Command Buffer No.:\t" << PJEngine::commandBuffers[i] << endl;
			return PJEngine::result;
		}

		renderPassBeginInfo.framebuffer = PJEngine::framebuffers[i];	// framebuffer that the current command buffer is associated with

		vkCmdBeginRenderPass(PJEngine::commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// BIND => decide for an pipeline and use it for graphical calculation
		vkCmdBindPipeline(PJEngine::commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PJEngine::pipeline);
		// DRAW => drawing on swapchain images
		vkCmdDraw(PJEngine::commandBuffers[i], 3, 1, 0, 1);				// TODO (hardcoded for current shader)

		vkCmdEndRenderPass(PJEngine::commandBuffers[i]);

		PJEngine::result = vkEndCommandBuffer(PJEngine::commandBuffers[i]);
		if (PJEngine::result != VK_SUCCESS) {
			cout << "Error at vkEndCommandBuffer of Command Buffer No.:\t" << PJEngine::commandBuffers[i] << endl;
			return PJEngine::result;
		}
	}

	/* Creating SEMAPHORES for drawFrameOnSurface() */

	VkSemaphoreCreateInfo semaphoreInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	PJEngine::result = vkCreateSemaphore(PJEngine::logicalDevice, &semaphoreInfo, nullptr, &PJEngine::semaphoreSwapchainImageReceived);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateSemaphore" << endl;
		return PJEngine::result;
	}
	PJEngine::result = vkCreateSemaphore(PJEngine::logicalDevice, &semaphoreInfo, nullptr, &PJEngine::semaphoreRenderingFinished);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateSemaphore" << endl;
		return PJEngine::result;
	}

	VkFenceCreateInfo fenceInfo;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

	PJEngine::result = vkCreateFence(PJEngine::logicalDevice, &fenceInfo, nullptr, &PJEngine::fenceRenderFinished);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateFence" << endl;
		return PJEngine::result;
	}

	return 0;
}

void stopVulkan() {
	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(PJEngine::logicalDevice);

	vkDestroyFence(PJEngine::logicalDevice, PJEngine::fenceRenderFinished, nullptr);
	vkDestroySemaphore(PJEngine::logicalDevice, PJEngine::semaphoreSwapchainImageReceived, nullptr);
	vkDestroySemaphore(PJEngine::logicalDevice, PJEngine::semaphoreRenderingFinished, nullptr);

	vkFreeCommandBuffers(PJEngine::logicalDevice, PJEngine::commandPool, PJEngine::numberOfImagesInSwapchain, PJEngine::commandBuffers);	// also automatically done by vkDestroyCommandPool
	delete[] PJEngine::commandBuffers;

	vkDestroyCommandPool(PJEngine::logicalDevice, PJEngine::commandPool, nullptr);

	/* CLEANUP => delete for all 'new' initializations */
	for (int i = 0; i < PJEngine::numberOfImagesInSwapchain; i++) {
		vkDestroyFramebuffer(PJEngine::logicalDevice, PJEngine::framebuffers[i], nullptr);
		vkDestroyImageView(PJEngine::logicalDevice, PJEngine::imageViews[i], nullptr);
	}
	delete[] PJEngine::framebuffers;
	delete[] PJEngine::imageViews;

	vkDestroyPipeline(PJEngine::logicalDevice, PJEngine::pipeline, nullptr);
	vkDestroyRenderPass(PJEngine::logicalDevice, PJEngine::renderPass, nullptr);
	vkDestroyPipelineLayout(PJEngine::logicalDevice, PJEngine::pipelineLayout, nullptr);
	vkDestroyShaderModule(PJEngine::logicalDevice, PJEngine::shaderModuleBasicVert, nullptr);
	vkDestroyShaderModule(PJEngine::logicalDevice, PJEngine::shaderModuleBasicFrag, nullptr);
	clearShaderStages();

	vkDestroySwapchainKHR(PJEngine::logicalDevice, PJEngine::swapchain, nullptr);
	vkDestroyDevice(PJEngine::logicalDevice, nullptr);
	vkDestroySurfaceKHR(PJEngine::vulkanInstance, PJEngine::surface, nullptr);
	vkDestroyInstance(PJEngine::vulkanInstance, nullptr);
}

/* drawFrameOnSurface has 3 steps => using semaphores
 * 1) get an image from swapchain
 * 2) render inside of selected image
 * 3) give back image to swapchain to present it
 */
void drawFrameOnSurface() {
	/* CPU waits here for SIGNALED fence(s)*/
	PJEngine::result = vkWaitForFences(
		PJEngine::logicalDevice, 
		1, 
		&PJEngine::fenceRenderFinished, 
		VK_TRUE, 
		numeric_limits<uint64_t>::max()
	);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkWaitForFences" << endl;
		return;
	}

	/* UNSIGNALS fence(s) */
	PJEngine::result = vkResetFences(
		PJEngine::logicalDevice, 
		1, 
		&PJEngine::fenceRenderFinished
	);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkResetFences" << endl;
		return;
	}

	uint32_t imageIndex;
	vkAcquireNextImageKHR(
		PJEngine::logicalDevice, 
		PJEngine::swapchain, 
		numeric_limits<uint64_t>::max(),			// timeout in ns before abort
		PJEngine::semaphoreSwapchainImageReceived,	// semaphore	=> only visible on GPU side
		VK_NULL_HANDLE,								// fences		=> like semaphores ; usable inside of Cpp Code
		&imageIndex
	);

	array<VkPipelineStageFlags, 1> waitStageMask {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT									// allows async rendering behavior
	};

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &PJEngine::semaphoreSwapchainImageReceived;
	submitInfo.pWaitDstStageMask = waitStageMask.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &PJEngine::commandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &PJEngine::semaphoreRenderingFinished;				// which semaphores will be triggered

	/* Submitting commandBuffer to queue => ACTUAL RENDERING */
	/* Fence must be unsignaled to proceed and SIGNALS fence */
	PJEngine::result = vkQueueSubmit(
		PJEngine::queueForPrototyping, 
		1, 
		&submitInfo, 
		PJEngine::fenceRenderFinished				// QUESTION => exactly 1 fence ???
	);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkQueueSubmit" << endl;
		return;
	}

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &PJEngine::semaphoreRenderingFinished;	// waiting for rendering to finish before presenting
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &PJEngine::swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;											// error code from different swapchains possible

	PJEngine::result = vkQueuePresentKHR(PJEngine::queueForPrototyping, &presentInfo);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkQueuePresentKHR" << endl;
		return;
	}
}

void loopVisualizationOf(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrameOnSurface();
	}
}

int main() {
	// MSVC isn't working correctly with __cplusplus
	cout << "C++-Version: " << __cplusplus << endl;

	int res;

	res = startGlfw3("Procedural Generation with Vulkan");
	if (res != 0) {
		return res;
	}

	res = startVulkan();
	if (res != 0) {
		stopGlfw3();
		return res;
	}

	loopVisualizationOf(PJEngine::window);

	stopVulkan();
	stopGlfw3();

	return 0;
}