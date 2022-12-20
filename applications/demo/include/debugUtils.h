/* Ignores header if not needed after #include */
    #pragma once
/* part of std => is_convertible_v */
	#include <cstdint>
	#include <type_traits>

	#include <vulkan/vulkan.h>

	#include <globalParams.h>
	#include <vef.h>

/* debugUtils for VK_EXT_debug_utils
*	debugPhysicalDeviceStats	=> prints useful information about GPUs
**	set_object_name				=> uses vef::vkSetDebugUtilsObjectNameEXT
**	get_object_type				=> verify the VKObjectType for set_object_name
*/
namespace pje {
	void debugPhysicalDeviceStats(VkPhysicalDevice& device);

	template <typename T>
	constexpr VkObjectType get_object_type(T handle = VK_NULL_HANDLE);

    template <typename T, bool enable_debug_func = true>
    VkResult set_object_name(VkDevice device, T object, const char* name, const void* next = nullptr);
}

// ################################################################################################################################################################## //

inline void pje::debugPhysicalDeviceStats(VkPhysicalDevice& physicalDevice) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	uint32_t apiVersion = properties.apiVersion;

	std::cout << "[OS] Available GPU:\t\t" << properties.deviceName << std::endl;
	std::cout << "\t\t\t\tVulkan API Version:\t" << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
	std::cout << "\t\t\t\tDevice Type:\t\t" << properties.deviceType << std::endl;

	/* FEATURES => what kind of shaders can be processed */
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	std::cout << "\t\t\t\tFeatures:" << std::endl;
	std::cout << "\t\t\t\t\tGeometry Shader:\t" << features.geometryShader << std::endl;

	/* memoryProperties => heaps/memory with flags */
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	uint64_t accHeap = 0;
	for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
		accHeap += memoryProperties.memoryHeaps[i].size;
	}

	std::cout << "\t\t\t\tMemory Properties:" << std::endl;
	std::cout << "\t\t\t\t\tTypeCount:\t" << memoryProperties.memoryTypeCount << std::endl;
	std::cout << "\t\t\t\t\tHeapCount:\t" << memoryProperties.memoryHeapCount << std::endl;
	std::cout << "\t\t\t\t\taccumulated Heap:\t" << accHeap << std::endl;

	/* GPUs have queues to solve tasks ; their respective attributes are clustered in families */
	uint32_t numberOfQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, nullptr);

	auto familyProperties = std::vector<VkQueueFamilyProperties>(numberOfQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numberOfQueueFamilies, familyProperties.data());

	/* displays attributes of each queue family and the amount of available queues */
	std::cout << "\t\t\t\tNumber of Queue Families:\t" << numberOfQueueFamilies << std::endl;
	for (uint32_t i = 0; i < numberOfQueueFamilies; i++) {
		std::cout << "\t\t\t\tQueue Family #" << i << std::endl;
		std::cout << "\t\t\t\t\tQueues in Family:\t\t" << familyProperties[i].queueCount << std::endl;
		/* BITWISE AND CHECK */
		std::cout << "\t\t\t\t\tVK_QUEUE_GRAPHICS_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
		std::cout << "\t\t\t\t\tVK_QUEUE_COMPUTE_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
		std::cout << "\t\t\t\t\tVK_QUEUE_TRANSFER_BIT\t\t" << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
		std::cout << "\t\t\t\t\tVK_QUEUE_SPARSE_BINDING_BIT\t" << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
	}

	/* SurfaceCapabilities => checks access to triple buffering etc. */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pje::context.surface, &surfaceCapabilities);
	std::cout << "\t\t\t\tSurface Capabilities:" << std::endl;
	std::cout << "\t\t\t\t\tminImageCount:\t\t" << surfaceCapabilities.minImageCount << std::endl;
	std::cout << "\t\t\t\t\tmaxImageCount:\t\t" << surfaceCapabilities.maxImageCount << std::endl;
	std::cout << "\t\t\t\t\tcurrentExtent:\t\t" << surfaceCapabilities.currentExtent.width << "x" << surfaceCapabilities.currentExtent.height << std::endl;

	/* SurfaceFormats => defines how colors are stored */
	uint32_t numberOfFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pje::context.surface, &numberOfFormats, nullptr);
	auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(numberOfFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pje::context.surface, &numberOfFormats, surfaceFormats.data());

	std::cout << "\t\t\t\tVkFormats: " << std::endl;
	for (uint32_t i = 0; i < numberOfFormats; i++) {
		std::cout << "\t\t\t\t\tIndex:\t\t" << surfaceFormats[i].format << std::endl;
	}

	/* PresentationMode => how CPU and GPU may interact with swapchain images */
	uint32_t numberOfPresentationModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pje::context.surface, &numberOfPresentationModes, nullptr);
	auto presentModes = std::vector<VkPresentModeKHR>(numberOfPresentationModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pje::context.surface, &numberOfPresentationModes, presentModes.data());

	/* Index 0 is necessary for immediate presentation */
	std::cout << "\t\t\t\tPresentation Modes:" << std::endl;
	for (uint32_t i = 0; i < numberOfPresentationModes; i++) {
		std::cout << "\t\t\t\t\tIndex:\t\t" << presentModes[i] << std::endl;
	}
}

template <typename T, bool enable_debug_func>
VkResult pje::set_object_name(VkDevice device, T object, const char* name, const void* next) {
    // do nothing in release mode
    if constexpr (enable_debug_func) {
		const VkDebugUtilsObjectNameInfoEXT name_info{
			VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			next,
			get_object_type<T>(),
			reinterpret_cast<std::uint64_t>(object),
			name
		};

		auto res = vef::vkSetDebugUtilsObjectNameEXT(device, &name_info);

		return res;
	}
	else {
		return VK_SUCCESS;
	}
}

template <typename T>
constexpr VkObjectType pje::get_object_type(T handle) {
	if constexpr (std::is_convertible_v<T, VkInstance>)
		return VkObjectType::VK_OBJECT_TYPE_INSTANCE;
	else if constexpr (std::is_convertible_v<T, VkPhysicalDevice>)
		return VkObjectType::VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	else if constexpr (std::is_convertible_v<T, VkDevice>)
		return VkObjectType::VK_OBJECT_TYPE_DEVICE;
	else if constexpr (std::is_convertible_v<T, VkQueue>)
		return VkObjectType::VK_OBJECT_TYPE_QUEUE;
	else if constexpr (std::is_convertible_v<T, VkSemaphore>)
		return VkObjectType::VK_OBJECT_TYPE_SEMAPHORE;
	else if constexpr (std::is_convertible_v<T, VkCommandBuffer>)
		return VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER;
	else if constexpr (std::is_convertible_v<T, VkFence>)
		return VkObjectType::VK_OBJECT_TYPE_FENCE;
	else if constexpr (std::is_convertible_v<T, VkDeviceMemory>)
		return VkObjectType::VK_OBJECT_TYPE_DEVICE_MEMORY;
	else if constexpr (std::is_convertible_v<T, VkBuffer>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER;
	else if constexpr (std::is_convertible_v<T, VkImage>)
		return VkObjectType::VK_OBJECT_TYPE_IMAGE;
	else if constexpr (std::is_convertible_v<T, VkEvent>)
		return VkObjectType::VK_OBJECT_TYPE_EVENT;
	else if constexpr (std::is_convertible_v<T, VkQueryPool>)
		return VkObjectType::VK_OBJECT_TYPE_QUERY_POOL;
	else if constexpr (std::is_convertible_v<T, VkBufferView>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER_VIEW;
	else if constexpr (std::is_convertible_v<T, VkImageView>)
		return VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW;
	else if constexpr (std::is_convertible_v<T, VkShaderModule>)
		return VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE;
	else if constexpr (std::is_convertible_v<T, VkPipelineCache>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE_CACHE;
	else if constexpr (std::is_convertible_v<T, VkPipelineLayout>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	else if constexpr (std::is_convertible_v<T, VkRenderPass>)
		return VkObjectType::VK_OBJECT_TYPE_RENDER_PASS;
	else if constexpr (std::is_convertible_v<T, VkPipeline>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE;
	else if constexpr (std::is_convertible_v<T, VkDescriptorSetLayout>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	else if constexpr (std::is_convertible_v<T, VkSampler>)
		return VkObjectType::VK_OBJECT_TYPE_SAMPLER;
	else if constexpr (std::is_convertible_v<T, VkDescriptorPool>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	else if constexpr (std::is_convertible_v<T, VkDescriptorSet>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET;
	else if constexpr (std::is_convertible_v<T, VkFramebuffer>)
		return VkObjectType::VK_OBJECT_TYPE_FRAMEBUFFER;
	else if constexpr (std::is_convertible_v<T, VkCommandPool>)
		return VkObjectType::VK_OBJECT_TYPE_COMMAND_POOL;
	else if constexpr (std::is_convertible_v<T, VkSamplerYcbcrConversion>)
		return VkObjectType::VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION;
	else if constexpr (std::is_convertible_v<T, VkDescriptorUpdateTemplate>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE;
	else if constexpr (std::is_convertible_v<T, VkPrivateDataSlot>)
		return VkObjectType::VK_OBJECT_TYPE_PRIVATE_DATA_SLOT;
	else if constexpr (std::is_convertible_v<T, VkSurfaceKHR>)
		return VkObjectType::VK_OBJECT_TYPE_SURFACE_KHR;
	else if constexpr (std::is_convertible_v<T, VkSwapchainKHR>)
		return VkObjectType::VK_OBJECT_TYPE_SWAPCHAIN_KHR;
	else if constexpr (std::is_convertible_v<T, VkDisplayKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DISPLAY_KHR;
	else if constexpr (std::is_convertible_v<T, VkDisplayModeKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DISPLAY_MODE_KHR;
	else if constexpr (std::is_convertible_v<T, VkDebugReportCallbackEXT>)
		return VkObjectType::VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
	else if constexpr (std::is_convertible_v<T, VkVideoSessionKHR>)
		return VkObjectType::VK_OBJECT_TYPE_VIDEO_SESSION_KHR;
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
	else if constexpr (std::is_convertible_v<T, VkVideoSessionParametersKHR>)
		return VkObjectType::VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR;
#endif
	else if constexpr (std::is_convertible_v<T, VkCuModuleNVX>)
		return VkObjectType::VK_OBJECT_TYPE_CU_MODULE_NVX;
	else if constexpr (std::is_convertible_v<T, VkCuFunctionNVX>)
		return VkObjectType::VK_OBJECT_TYPE_CU_FUNCTION_NVX;
	else if constexpr (std::is_convertible_v<T, VkDebugUtilsMessengerEXT>)
		return VkObjectType::VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
	else if constexpr (std::is_convertible_v<T, VkAccelerationStructureKHR>)
		return VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
	else if constexpr (std::is_convertible_v<T, VkValidationCacheEXT>)
		return VkObjectType::VK_OBJECT_TYPE_VALIDATION_CACHE_EXT;
	else if constexpr (std::is_convertible_v<T, VkAccelerationStructureNV>)
		return VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV;
	else if constexpr (std::is_convertible_v<T, VkPerformanceConfigurationINTEL>)
		return VkObjectType::VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL;
	else if constexpr (std::is_convertible_v<T, VkDeferredOperationKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR;
	else if constexpr (std::is_convertible_v<T, VkIndirectCommandsLayoutNV>)
		return VkObjectType::VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV;
	/*else if constexpr (std::is_convertible_v<T, VkBufferCollectionFUCHSIA>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA;*/
		/*else if constexpr (std::is_convertible_v<T, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR;
		else if constexpr (std::is_convertible_v<T, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR;
		else if constexpr (std::is_convertible_v<T, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT;*/
	else
		return VkObjectType::VK_OBJECT_TYPE_UNKNOWN;
}