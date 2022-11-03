/* Ignores header if not needed after #include */
    #pragma once
/* part of std => is_convertible_v*/
	#include <cstdint>
	#include <type_traits>

	#include <vef.h>
	#include <vulkan/vulkan.h>

/* debugUtils for VK_EXT_debug_utils
**	set_object_name => uses vef::vkSetDebugUtilsObjectNameEXT
**	get_object_type => verify the VKObjectType for set_object_name
*/
namespace pje {
	void debugPhysicalDeviceStats(VkPhysicalDevice& device);

	template <typename VulkanHandle>
	inline constexpr VkObjectType get_object_type(VulkanHandle handle = VK_NULL_HANDLE);

    template <typename VulkanObjectType, bool enable_debug_func = true>
    VkResult set_object_name(VkDevice device, VulkanObjectType object, const char* name, const void* next = nullptr);
}

template <typename VulkanObjectType, bool enable_debug_func>
VkResult pje::set_object_name(VkDevice device, VulkanObjectType object, const char* name, const void* next) {
    // do nothing in release mode
    if constexpr (enable_debug_func) {
		const VkDebugUtilsObjectNameInfoEXT name_info{
			VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			next,
			get_object_type<VulkanObjectType>(),
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

void debugPhysicalDeviceStats(VkPhysicalDevice& device) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	uint32_t apiVersion = properties.apiVersion;

	std::cout << "[OS] Available GPU:\t\t" << properties.deviceName << std::endl;
	std::cout << "\t\t\t\tVulkan API Version:\t" << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << std::endl;
	std::cout << "\t\t\t\tDevice Type:\t\t" << properties.deviceType << std::endl;
	std::cout << "\t\t\t\tdiscreteQueuePriorities:\t" << properties.limits.discreteQueuePriorities << std::endl;

	/* FEATURES => what kind of shaders can be processed */
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	std::cout << "\t\t\t\tFeatures:" << std::endl;
	std::cout << "\t\t\t\t\tGeometry Shader:\t" << features.geometryShader << std::endl;

	/* memoryProperties => heaps/memory with flags */
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
	std::cout << "\t\t\t\tMemory Properties:" << std::endl;
	std::cout << "\t\t\t\t\tHeaps:\t" << memoryProperties.memoryHeaps->size << std::endl;

	/* GPUs have queues to solve tasks ; their respective attributes are clustered in families */
	uint32_t numberOfQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, nullptr);

	auto familyProperties = std::vector<VkQueueFamilyProperties>(numberOfQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, familyProperties.data());

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
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, pje::surface, &surfaceCapabilities);
	std::cout << "\t\t\t\tSurface Capabilities:" << std::endl;
	std::cout << "\t\t\t\t\tminImageCount:\t\t" << surfaceCapabilities.minImageCount << std::endl;
	std::cout << "\t\t\t\t\tmaxImageCount:\t\t" << surfaceCapabilities.maxImageCount << std::endl;
	std::cout << "\t\t\t\t\tcurrentExtent:\t\t" << surfaceCapabilities.currentExtent.width << "x" << surfaceCapabilities.currentExtent.height << std::endl;

	/* SurfaceFormats => defines how colors are stored */
	uint32_t numberOfFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, pje::surface, &numberOfFormats, nullptr);
	auto surfaceFormats = std::vector<VkSurfaceFormatKHR>(numberOfFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, pje::surface, &numberOfFormats, surfaceFormats.data());

	std::cout << "\t\t\t\tVkFormats: " << std::endl;
	for (uint32_t i = 0; i < numberOfFormats; i++) {
		std::cout << "\t\t\t\t\tIndex:\t\t" << surfaceFormats[i].format << std::endl;
	}

	/* PresentationMode => how CPU and GPU may interact with swapchain images */
	uint32_t numberOfPresentationModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, pje::surface, &numberOfPresentationModes, nullptr);
	auto presentModes = std::vector<VkPresentModeKHR>(numberOfPresentationModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, pje::surface, &numberOfPresentationModes, presentModes.data());

	/* Index 0 is necessary for immediate presentation */
	std::cout << "\t\t\t\tPresentation Modes:" << std::endl;
	for (uint32_t i = 0; i < numberOfPresentationModes; i++) {
		std::cout << "\t\t\t\t\tIndex:\t\t" << presentModes[i] << std::endl;
	}
}

template <typename VulkanHandle>
inline constexpr VkObjectType pje::get_object_type(VulkanHandle handle) {
	if constexpr (std::is_convertible_v<VulkanHandle, VkInstance>)
		return VkObjectType::VK_OBJECT_TYPE_INSTANCE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPhysicalDevice>)
		return VkObjectType::VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDevice>)
		return VkObjectType::VK_OBJECT_TYPE_DEVICE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkQueue>)
		return VkObjectType::VK_OBJECT_TYPE_QUEUE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkSemaphore>)
		return VkObjectType::VK_OBJECT_TYPE_SEMAPHORE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkCommandBuffer>)
		return VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkFence>)
		return VkObjectType::VK_OBJECT_TYPE_FENCE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDeviceMemory>)
		return VkObjectType::VK_OBJECT_TYPE_DEVICE_MEMORY;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkBuffer>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkImage>)
		return VkObjectType::VK_OBJECT_TYPE_IMAGE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkEvent>)
		return VkObjectType::VK_OBJECT_TYPE_EVENT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkQueryPool>)
		return VkObjectType::VK_OBJECT_TYPE_QUERY_POOL;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkBufferView>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER_VIEW;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkImageView>)
		return VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkShaderModule>)
		return VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPipelineCache>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE_CACHE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPipelineLayout>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkRenderPass>)
		return VkObjectType::VK_OBJECT_TYPE_RENDER_PASS;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPipeline>)
		return VkObjectType::VK_OBJECT_TYPE_PIPELINE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDescriptorSetLayout>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkSampler>)
		return VkObjectType::VK_OBJECT_TYPE_SAMPLER;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDescriptorPool>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDescriptorSet>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkFramebuffer>)
		return VkObjectType::VK_OBJECT_TYPE_FRAMEBUFFER;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkCommandPool>)
		return VkObjectType::VK_OBJECT_TYPE_COMMAND_POOL;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkSamplerYcbcrConversion>)
		return VkObjectType::VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDescriptorUpdateTemplate>)
		return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPrivateDataSlot>)
		return VkObjectType::VK_OBJECT_TYPE_PRIVATE_DATA_SLOT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkSurfaceKHR>)
		return VkObjectType::VK_OBJECT_TYPE_SURFACE_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkSwapchainKHR>)
		return VkObjectType::VK_OBJECT_TYPE_SWAPCHAIN_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDisplayKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DISPLAY_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDisplayModeKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DISPLAY_MODE_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDebugReportCallbackEXT>)
		return VkObjectType::VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
	else if constexpr (std::is_convertible_v<VulkanHandle, VkVideoSessionKHR>)
		return VkObjectType::VK_OBJECT_TYPE_VIDEO_SESSION_KHR;
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
	else if constexpr (std::is_convertible_v<VulkanHandle, VkVideoSessionParametersKHR>)
		return VkObjectType::VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR;
#endif
	else if constexpr (std::is_convertible_v<VulkanHandle, VkCuModuleNVX>)
		return VkObjectType::VK_OBJECT_TYPE_CU_MODULE_NVX;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkCuFunctionNVX>)
		return VkObjectType::VK_OBJECT_TYPE_CU_FUNCTION_NVX;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDebugUtilsMessengerEXT>)
		return VkObjectType::VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkAccelerationStructureKHR>)
		return VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkValidationCacheEXT>)
		return VkObjectType::VK_OBJECT_TYPE_VALIDATION_CACHE_EXT;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkAccelerationStructureNV>)
		return VkObjectType::VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkPerformanceConfigurationINTEL>)
		return VkObjectType::VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkDeferredOperationKHR>)
		return VkObjectType::VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR;
	else if constexpr (std::is_convertible_v<VulkanHandle, VkIndirectCommandsLayoutNV>)
		return VkObjectType::VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV;
	/*else if constexpr (std::is_convertible_v<VulkanHandle, VkBufferCollectionFUCHSIA>)
		return VkObjectType::VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA;*/
		/*else if constexpr (std::is_convertible_v<VulkanHandle, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR;
		else if constexpr (std::is_convertible_v<VulkanHandle, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR;
		else if constexpr (std::is_convertible_v<VulkanHandle, VkInstance>)
			return VkObjectType::VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT;*/
	else
		return VkObjectType::VK_OBJECT_TYPE_UNKNOWN;
}