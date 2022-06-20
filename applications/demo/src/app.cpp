#include <app.h>

namespace PJEngine {
	VkResult	result;

	VkInstance		vulkanInstance;
	VkSurfaceKHR	surface;
	VkDevice		logicalDevice;

	VkSwapchainKHR	swapchain;
	uint32_t		numberOfImagesInSwapchain = 0;
	VkImageView*	imageViews;

	VkShaderModule	shaderModuleBasicVert;
	VkShaderModule	shaderModuleBasicFrag;
	std::vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;

	GLFWwindow*		window;
	const uint32_t	WINDOW_WIDTH = 1280;
	const uint32_t	WINDOW_HEIGHT = 720;
}

using namespace std;

void printPhysicalDeviceStats(VkPhysicalDevice &device) {
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

	cout << "\t\t\t\tPresentation Modes:" << endl;
	for (int i = 0; i < numberOfPresentationModes; i++) {
		cout << "\t\t\t\t\tIndex:\t\t" << presentModes[i] << endl;
	}
}

int startGlfw3(const char *windowName) {
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

vector<char> readSpirvFile(const string &filename) {
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
void createShaderModule(const vector<char> &spvCode, VkShaderModule &shaderModule) {
	VkShaderModuleCreateInfo shaderModuleInfo;
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.pNext = nullptr;
	shaderModuleInfo.flags = 0;
	shaderModuleInfo.codeSize = spvCode.size();
	shaderModuleInfo.pCode = (uint32_t*)spvCode.data();	// spv instructions always have a size of 32 bits, so casting is possible

	PJEngine::result = vkCreateShaderModule(PJEngine::logicalDevice, &shaderModuleInfo, nullptr, &shaderModule);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateShaderModule" << endl;
		throw runtime_error("Failed to create VkShaderModule");
	}
}

/* Adds Element to PJEngine::shaderStageInfos */
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
	appInfo.pNext = nullptr;										// for Vulkan extensions
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

	// OLD:		VkLayerProperties *layers = new VkLayerProperties[numberOfLayers];
	auto availableLayers = vector<VkLayerProperties>(numberOfLayers);
	vkEnumerateInstanceLayerProperties(&numberOfLayers, availableLayers.data());

	cout << "Number of Instance Layers:\t" << numberOfLayers << endl;
	for (int i = 0; i < numberOfLayers; i++) {
		cout << "\tLayer Name:\t" << availableLayers[i].layerName << endl;
		cout << "\t\t\t" << availableLayers[i].description << endl;
	}

	/* EXTENSIONS add functions like actually displaying rendered images	*/
	/* extensions can be part of a layer									*/
	uint32_t numberOfExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, nullptr);

	// OLD:		VkExtensionProperties *extensions = new VkExtensionProperties[numberOfExtensions];
	auto availableExtensions = vector<VkExtensionProperties>(numberOfExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &numberOfExtensions, availableExtensions.data());

	cout << "Number of Instance Extensions:\t" << numberOfExtensions << endl;
	for (int i = 0; i < numberOfExtensions; i++) {
		cout << "\tExtension:\t" << availableExtensions[i].extensionName << endl;
	}

	/* gets all EXTENSIONS GLFW needs on current OS */
	uint32_t numberOfRequiredGlfwExtensions = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&numberOfRequiredGlfwExtensions);

	cout << "numberOfGlfwExtensions:\t" << numberOfRequiredGlfwExtensions << endl;

	// TODO ( does this layer always exist? )
	const vector<const char*> usedInstanceLayers {
		"VK_LAYER_KHRONOS_validation",
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
	PJEngine::result = glfwCreateWindowSurface(PJEngine::vulkanInstance, PJEngine::window, nullptr, &PJEngine::surface);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at glfwCreateWindowSurface" << endl;
		return PJEngine::result;
	}

	/* Vulkan differs between PHYSICAL and logical GPU reference */
	/* A physical reference is necessary for a logical reference */
	uint32_t numberOfPhysicalDevices = 0;

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter):
		1) get number of GPUs and stores value in 2nd parameter */
	vkEnumeratePhysicalDevices(PJEngine::vulkanInstance, &numberOfPhysicalDevices, nullptr);

	// OLD:		VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[numberOfPhysicalDevices];
	auto physicalDevices = vector<VkPhysicalDevice>(numberOfPhysicalDevices);

	/*	vkEnumeratePhysicalDevices has two functionalities:
		2) get reference for a number of GPUs and store them in an array of physical devices */
	vkEnumeratePhysicalDevices(PJEngine::vulkanInstance, &numberOfPhysicalDevices, physicalDevices.data());

	cout << "Number of GPUs:\t\t" << numberOfPhysicalDevices << endl;

	/* Shows some properties and features for each available GPU */
	for (int i = 0; i < numberOfPhysicalDevices; i++) {
		printPhysicalDeviceStats(physicalDevices[i]);
	}

	/* deviceQueueCreateInfo used for deviceInfo */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext = nullptr;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = 0;										// TODO ( analyse available families before setting one )
	deviceQueueInfo.queueCount = 1;												// TODO ( analyse available queues in family first )
	deviceQueueInfo.pQueuePriorities = new float[] {1.0f};

	VkPhysicalDeviceFeatures usedFeatures = {};									// all possible features set to false

	/* EXTENSIONS on device level */
	const vector<const char*> deviceExtensions {
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

	/* Getting Queue of some logical device to assign tasks later */
	VkQueue queueOne;
	vkGetDeviceQueue(PJEngine::logicalDevice, 0, 0, &queueOne);

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
	swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;							// TODO ( choose dynamically )
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
	imageViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;								// TODO ( choose dynamically )
	imageViewInfo.components = VkComponentMapping {
		VK_COMPONENT_SWIZZLE_IDENTITY // properties r,g,b,a stay as their identity 
	};
	imageViewInfo.subresourceRange = VkImageSubresourceRange {
		VK_IMAGE_ASPECT_COLOR_BIT,	// aspectMask : describes what kind of data is stored in image
		0,							// baseMipLevel
		1,							// levelCount : describes how many MipMap levels exist
		0,							// baseArrayLayer : for VR
		1							// layerCount : for VR
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
	cout << "\n[DEBUG] Stages in PJEngine::shaderStageInfos: " << PJEngine::shaderStageInfos.size() << endl;

	/* Viewport for ???imageView??? */
	VkViewport viewport;
	viewport.x = 0.0f;	// upper left corner
	viewport.y = 0.0f;	// upper left corner
	viewport.width = PJEngine::WINDOW_WIDTH;
	viewport.height = PJEngine::WINDOW_HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/* PIPELINE BUILDING */
	/* Shader pipelines consist out of fixed functions and programmable functions (shaders) */

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

	/* Fixed Function : Input Assembler */
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// how single vertices will be assembled
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(PJEngine::logicalDevice);

	return 0;
}

void stopVulkan() {
	/* CLEANUP => delete for all new initializations */
	for (int i = 0; i < PJEngine::numberOfImagesInSwapchain; i++) {
		vkDestroyImageView(PJEngine::logicalDevice, PJEngine::imageViews[i], nullptr);
	}
	delete[] PJEngine::imageViews;

	vkDestroyShaderModule(PJEngine::logicalDevice, PJEngine::shaderModuleBasicVert, nullptr);
	vkDestroyShaderModule(PJEngine::logicalDevice, PJEngine::shaderModuleBasicFrag, nullptr);
	clearShaderStages();

	vkDestroySwapchainKHR(PJEngine::logicalDevice, PJEngine::swapchain, nullptr);
	vkDestroyDevice(PJEngine::logicalDevice, nullptr);
	vkDestroySurfaceKHR(PJEngine::vulkanInstance, PJEngine::surface, nullptr);
	vkDestroyInstance(PJEngine::vulkanInstance, nullptr);
}

void loopVisualizationOf(GLFWwindow *window) {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

int main() {
	cout << "C++-Version: " << __cplusplus << endl;			// guess:	MSVC isn't working correctly with __cplusplus

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