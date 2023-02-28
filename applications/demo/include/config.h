/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>				// uintXXX_t
	#include <vulkan/vulkan.h>
	#include <glm/gtc/matrix_transform.hpp>

namespace pje::config {
	inline constexpr	char*							appName							= "PJEngine";

	inline constexpr	VkPhysicalDeviceType			preferredPhysicalDeviceType		= VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	inline constexpr	std::array<VkQueueFlagBits,1>	neededFamilyQueueAttributes		= { VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT };
	inline constexpr	const uint32_t					neededSurfaceImages				= 2;
	inline constexpr	VkPresentModeKHR				neededPresentationMode			= VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;

	inline constexpr	VkFormat						outputFormat					= VkFormat::VK_FORMAT_B8G8R8A8_UNORM;		// VkFormat 44
	inline constexpr	VkFormat						depthFormat						= VkFormat::VK_FORMAT_D32_SFLOAT;			// 32 bits for depth component
	inline constexpr	float							clearValueDefault[4]			= { 0.0f, 0.0f, 0.0f, 1.0f };				// DEFAULT BACKGROUND;

	inline constexpr	VkSampleCountFlagBits			msaaFactor						= VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT;

	inline constexpr	uint32_t						initWindowWidth					= 1280;		// TODO
	inline constexpr	uint32_t						initWindowHeight				= 720;		// TODO

	inline constexpr	size_t							selectedPJModel					= 2;
	inline constexpr	float							antiFbxScale					= 0.01f;

	inline const		glm::mat4						initCameraPose					= glm::inverse(glm::lookAt(glm::vec3(4.0f, 2.0f, 4.0f), 
																													glm::vec3(0.0f, 0.0f, 0.0f), 
																													glm::vec3(0.0f, 1.0f, 0.0f)));
	inline constexpr	float							nearPlane						= 0.1f;
	inline constexpr	float							farPlane						= 100.0f;
}