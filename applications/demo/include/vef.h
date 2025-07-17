/* Ignores header if not needed after #include */
	#pragma once

	#include <vulkan/vulkan.h>

/* Vulkan Extension Functions => vef::someEXT(parameters) to use in code */
namespace vef {
	extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
}