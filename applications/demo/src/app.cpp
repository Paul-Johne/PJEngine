#include <app.h>

using namespace std;

VkInstance vulkanInstance;
VkResult result;

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
	vkGetPhysicalDeviceQueueFamilyProperties(*device, &numberOfQueueFamilies, NULL);

	VkQueueFamilyProperties *familyProperties = new VkQueueFamilyProperties[numberOfQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(*device, &numberOfQueueFamilies, familyProperties);

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
	delete[] familyProperties;
}

int main() {

	/* appInfo used for instanceInfo */
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;				// sType for driver
	appInfo.pNext = NULL;											// for Vulkan extensions
	appInfo.pApplicationName = "Procedural Generation with Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PJ Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	cout << "Engine started.." << endl;

	/* LAYERS are additional features between Vulkan functions	*/
	/* which layers are available depends on the GPU			*/
	uint32_t numberOfLayers = 0;
	vkEnumerateInstanceLayerProperties(&numberOfLayers, NULL);

	VkLayerProperties *layers = new VkLayerProperties[numberOfLayers];
	vkEnumerateInstanceLayerProperties(&numberOfLayers, layers);

	cout << "Number of Instance Layers:\t" << numberOfLayers << endl;
	for (int i = 0; i < numberOfLayers; i++) {
		cout << "\tLayer Name:\t" << layers[i].layerName << endl;
		cout << "\t\t\t" << layers[i].description << endl;
	}

	/* EXTENSIONS add functions like actually displaying rendered images	*/
	/* extensions can be part of a layer									*/
	uint32_t numberOfExtensions = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &numberOfExtensions, NULL);

	VkExtensionProperties *extensions = new VkExtensionProperties[numberOfExtensions];
	vkEnumerateInstanceExtensionProperties(NULL, &numberOfExtensions, extensions);

	cout << "Number of Instance Extensions:\t" << numberOfExtensions << endl;
	for (int i = 0; i < numberOfExtensions; i++) {
		cout << "\tExtension:\t" << extensions[i].extensionName << endl;
	}

	const vector<const char*> usedInstanceLayers {
		"VK_LAYER_KHRONOS_validation"	// TODO ( does this layer always exist? )
	};

	/* instanceInfo used for the actual Vulkan instance */
	VkInstanceCreateInfo instanceInfo;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = NULL;
	instanceInfo.flags = 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = usedInstanceLayers.size();
	instanceInfo.ppEnabledLayerNames = usedInstanceLayers.data();
	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = NULL;

	/* returns an enum for errorhandling ( e.g. VK_ERROR_OUT_OF DEVICE_MEMORY ) */
	result = vkCreateInstance(&instanceInfo, NULL, &vulkanInstance);

	if (result != VK_SUCCESS)
		return -1;

	/* Vulkan differs between PHYSICAL and logical GPU reference */
	/* A physical reference is necessary for a logical reference */
	uint32_t numberOfPhysicalDevices = 0;

	/*	vkEnumeratePhysicalDevices has two functionalities (depending on 3rd parameter): 
		1) get number of GPUs and stores value in 2nd parameter */
	vkEnumeratePhysicalDevices(vulkanInstance, &numberOfPhysicalDevices, NULL);

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
	deviceQueueInfo.pNext = NULL;
	deviceQueueInfo.flags = 0;
	deviceQueueInfo.queueFamilyIndex = 0;										// TODO ( analyse available families before setting one )
	deviceQueueInfo.queueCount = 4;												// TODO ( analyse available queues in family first )
	deviceQueueInfo.pQueuePriorities = new float[] {1.0f, 1.0f, 1.0f, 1.0f};

	VkPhysicalDeviceFeatures usedFeatures = {};									// all possible features set to false

	/* deviceInfo declares what resources will be claimed */
	/* deviceInfo is necessary for a logical reference    */
	VkDeviceCreateInfo deviceInfo;
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = NULL;
	deviceInfo.flags = 0;
	deviceInfo.queueCreateInfoCount = 1;										// number of VkDeviceQueueCreateInfo
	deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = NULL;
	deviceInfo.enabledExtensionCount = 0;
	deviceInfo.ppEnabledExtensionNames = NULL;
	deviceInfo.pEnabledFeatures = &usedFeatures;

	/* LOGICAL GPU reference */
	VkDevice logicalDevice;
	result = vkCreateDevice(physicalDevices[0], &deviceInfo, NULL, &logicalDevice);	// TODO ( choose device dynamically )

	if (result != VK_SUCCESS)
		return -1;

	/* Waiting for Vulkan API to finish all its tasks */
	vkDeviceWaitIdle(logicalDevice);
	/* CLEANUP */
	vkDestroyDevice(logicalDevice, NULL);
	vkDestroyInstance(vulkanInstance, NULL);
	/* using vector instead of new, so variable is on stack and will be automatically deleted in C++ */
	delete[] layers;
	delete[] extensions;
	//delete[] physicalDevices;

	return 0;
}