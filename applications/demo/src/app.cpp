#include <app.h>

namespace PJEngine {
	VkInstance vulkanInstance;
	VkSurfaceKHR surface;
	VkDevice logicalDevice;
	VkResult result;
	GLFWwindow* window;
}

using namespace std;

void printPhysicalDeviceStats(VkPhysicalDevice *device) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(*device, &properties);

	uint32_t apiVersion = properties.apiVersion;

	cout << "Available GPU:\t\t" << properties.deviceName << endl;
	cout << "\t\t\t\tVulkan API Version:\t" << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << endl;
	cout << "\t\t\t\tDevice Type:\t\t" << properties.deviceType << endl;
	cout << "\t\t\t\tdiscreteQueuePriorities:\t" << properties.limits.discreteQueuePriorities << endl;

	/* FEATURES => what kind of shaders can be processed */
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(*device, &features);
	cout << "\t\t\t\tFeatures:" << endl;
	cout << "\t\t\t\t\tGeometry Shader:\t" << features.geometryShader << endl;

	/* memoryProperties => heaps/memory with flags */
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(*device, &memoryProperties);
	cout << "\t\t\t\tMemory Properties:" << endl;
	cout << "\t\t\t\t\tHeaps:\t" << memoryProperties.memoryHeaps << endl;

	/* GPUs have queues to solve tasks ; their respective attributes are clustered in families */
	uint32_t numberOfQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(*device, &numberOfQueueFamilies, nullptr);

	// OLD:		VkQueueFamilyProperties *familyProperties = new VkQueueFamilyProperties[numberOfQueueFamilies];
	auto familyProperties = vector<VkQueueFamilyProperties>(numberOfQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(*device, &numberOfQueueFamilies, familyProperties.data());

	cout << "\t\t\t\tNumber of Queue Families:\t" << numberOfQueueFamilies << endl;

	for (int i = 0; i < numberOfQueueFamilies; i++) {
		cout << "\t\t\t\tQueue Family #" << i << endl;
		cout << "\t\t\t\t\tQueues in Family:\t\t" << familyProperties[i].queueCount << endl;
		/* bitwise AND check */
		cout << "\t\t\t\t\tVK_QUEUE_GRAPHICS_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_COMPUTE_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_TRANSFER_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << endl;
		cout << "\t\t\t\t\tVK_QUEUE_SPARSE_BINDING_BIT\t" << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << endl;
	}

	/* CLEANUP */
	/* using vector instead of new, so variable is on stack and will be automatically deleted in C++ */
	// delete[] familyProperties;
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

	PJEngine::window = glfwCreateWindow(1280, 720, windowName, nullptr, nullptr);

	return 0;
}

void stopGlfw3() {
	glfwDestroyWindow(PJEngine::window);
	glfwTerminate();
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

	/* TODO ( does this layer always exist? )
	const vector<const char*> usedInstanceExtensions {
		"VK_KHR_surface",
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};*/

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
		
	/* VkSurfaceKHR */
	PJEngine::result = glfwCreateWindowSurface(PJEngine::vulkanInstance, PJEngine::window, nullptr, &PJEngine::surface);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at glfwCreateWindowSurface" << endl;
		return PJEngine::result;
	}

	/*
	VkWin32SurfaceCreateInfoKHR win32SurfaceInfo;
	win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;	// TODO ( DISPLAY instead of WIN32 )
	win32SurfaceInfo.pNext = nullptr;
	win32SurfaceInfo.flags = 0;
	win32SurfaceInfo.hinstance = nullptr;
	win32SurfaceInfo.hwnd = nullptr;
	*/

	/* crossplatform surface
	PJEngine::result = vkCreateWin32SurfaceKHR(PJEngine::vulkanInstance, &win32SurfaceInfo, nullptr, &PJEngine::surface);
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateWin32SurfaceKHR" << endl;
		return PJEngine::result;
	}
	*/

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
		printPhysicalDeviceStats(&physicalDevices[i]);
	}

	/* deviceQueueCreateInfo used for deviceInfo */
	VkDeviceQueueCreateInfo deviceQueueInfo;
	deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueInfo.pNext = nullptr;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = 0;										// TODO ( analyse available families before setting one )
	deviceQueueInfo.queueCount = 2;												// TODO ( analyse available queues in family first )
	deviceQueueInfo.pQueuePriorities = new float[] {1.0f, 1.0f};

	VkPhysicalDeviceFeatures usedFeatures = {};									// all possible features set to false

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
	deviceInfo.enabledExtensionCount = 0;
	deviceInfo.ppEnabledExtensionNames = nullptr;
	deviceInfo.pEnabledFeatures = &usedFeatures;

	/* LOGICAL GPU reference */
	PJEngine::result = vkCreateDevice(physicalDevices[0], &deviceInfo, nullptr, &PJEngine::logicalDevice);	// TODO ( choose GPU dynamically )
	if (PJEngine::result != VK_SUCCESS) {
		cout << "Error at vkCreateDevice" << endl;
		return PJEngine::result;
	}

	/* EXAMPLE: Getting Queue of some logical device to set tasks */
	VkQueue queueOne;
	vkGetDeviceQueue(PJEngine::logicalDevice, 0, 0, &queueOne);

	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(PJEngine::logicalDevice);

	/* using vector instead of new, so variable is on stack and will be automatically deleted in C++ */
	//delete[] layers;
	//delete[] extensions;
	//delete[] physicalDevices;

	return 0;
}

void stopVulkan() {
	/* CLEANUP */
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