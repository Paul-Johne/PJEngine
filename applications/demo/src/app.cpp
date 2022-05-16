#include <app.h>

using namespace std;

VkInstance vulkanInstance;
VkSurfaceKHR surface;
VkDevice logicalDevice;
VkResult result;
GLFWwindow *window;

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

void startGlfw3() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(400, 350, "NICE", nullptr, nullptr);
}

void stopGlfw3() {
	glfwDestroyWindow(window);
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

	// TODO ( does this layer always exist? )
	const vector<const char*> usedInstanceLayers{
		"VK_LAYER_KHRONOS_validation",
	};
	// TODO ( does this layer always exist? )
	const vector<const char*> usedInstanceExtensions{
		"VK_KHR_surface",
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};

	/* instanceInfo used for the actual Vulkan instance */
	VkInstanceCreateInfo instanceInfo;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nullptr;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = usedInstanceLayers.size();
	instanceInfo.ppEnabledLayerNames = usedInstanceLayers.data();
	instanceInfo.enabledExtensionCount = usedInstanceExtensions.size();
	instanceInfo.ppEnabledExtensionNames = usedInstanceExtensions.data();

	/* returns an enum for errorhandling ( e.g. VK_ERROR_OUT_OF DEVICE_MEMORY ) */
	result = vkCreateInstance(&instanceInfo, nullptr, &vulkanInstance);
	if (result != VK_SUCCESS)
		return -1;

	/* VkSurfaceKHR */
	VkWin32SurfaceCreateInfoKHR win32SurfaceInfo;
	win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;	// TODO ( DISPLAY instead of WIN32 )
	win32SurfaceInfo.pNext = nullptr;
	win32SurfaceInfo.flags = 0;
	win32SurfaceInfo.hinstance = nullptr;
	win32SurfaceInfo.hwnd = nullptr;

	/* crossplatform surface */
	result = vkCreateWin32SurfaceKHR(vulkanInstance, &win32SurfaceInfo, nullptr, &surface);
	if (result != VK_SUCCESS)
		return -2;

	/* Vulkan differs between PHYSICAL and logical GPU reference */
	/* A physical reference is necessary for a logical reference */
	uint32_t numberOfPhysicalDevices = 0;

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter):
		1) get number of GPUs and stores value in 2nd parameter */
	vkEnumeratePhysicalDevices(vulkanInstance, &numberOfPhysicalDevices, nullptr);

	// OLD:		VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[numberOfPhysicalDevices];
	auto physicalDevices = vector<VkPhysicalDevice>(numberOfPhysicalDevices);

	/*	vkEnumeratePhysicalDevices has two functionalities:
		2) get reference for a number of GPUs and store them in an array of physical devices */
	vkEnumeratePhysicalDevices(vulkanInstance, &numberOfPhysicalDevices, physicalDevices.data());

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
	result = vkCreateDevice(physicalDevices[0], &deviceInfo, nullptr, &logicalDevice);	// TODO ( choose GPU dynamically )
	if (result != VK_SUCCESS)
		return -3;

	/* EXAMPLE: Getting Queue of some logical device to set tasks */
	VkQueue queueOne;
	vkGetDeviceQueue(logicalDevice, 0, 0, &queueOne);

	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(logicalDevice);

	/* using vector instead of new, so variable is on stack and will be automatically deleted in C++ */
	//delete[] layers;
	//delete[] extensions;
	//delete[] physicalDevices;

	return 0;
}

void stopVulkan() {
	/* CLEANUP */
	vkDestroyDevice(logicalDevice, nullptr);
	vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
	vkDestroyInstance(vulkanInstance, nullptr);
}

void loopGame() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

int main() {
	startGlfw3();

	if (startVulkan() != 0)
		return -1;

	loopGame();

	stopVulkan();

	stopGlfw3();

	return 0;
}