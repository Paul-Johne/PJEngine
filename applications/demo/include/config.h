/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>
	#include <vulkan/vulkan.h>
	#include <glm/gtc/matrix_transform.hpp>

/* Comment out to turn off */
#define ACTIVATE_ENGINE_GUI
#define ACTIVATE_DEPTH_TEST

namespace pje::config {
	inline constexpr	char*							appName							= "PJEngine";

	inline constexpr	VkPhysicalDeviceType			preferredPhysicalDeviceType		= VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;		// Default : external GPU
	inline constexpr	std::array<VkQueueFlagBits,1>	neededFamilyQueueAttributes		= { VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT };
	inline constexpr	const uint32_t					neededSurfaceImages				= 2;

	inline constexpr	VkFormat						outputFormat					= VkFormat::VK_FORMAT_B8G8R8A8_UNORM;								// Intended default VkFormat : 44
	inline constexpr	VkFormat						depthFormat						= VkFormat::VK_FORMAT_D32_SFLOAT;									// 32 bits for depth component
	inline constexpr	float							clearValueDefault[4]			= { 0.10f, 0.11f, 0.14f, 1.0f };									// Default Background

	inline constexpr	VkSampleCountFlagBits			plainImageFactor				= VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	inline constexpr	VkSampleCountFlagBits			msaaFactor						= VkSampleCountFlagBits::VK_SAMPLE_COUNT_4_BIT;

	inline constexpr	bool							enableVSync						= true;
	inline constexpr	bool							enableAnisotropy				= true;																// enables 2x Anisotropy

	inline constexpr	uint32_t						initWindowWidth					= 1280;
	inline constexpr	uint32_t						initWindowHeight				= 720;

	inline constexpr	size_t							selectedPJModel					= 2;																// ISSUE(only pje_chilli_plant.fbx is working at the moment)
	inline constexpr	float							antiFbxScale					= 0.01f;

	inline const		glm::mat4						initCameraPose					= glm::inverse(glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f),				// Position in World Space
																												   glm::vec3(0.0f, 0.0f, 0.0f),				// Point to look at
																												   glm::vec3(0.0f, 1.0f, 0.0f)));			// Up Vector of Camera
	inline constexpr	float							nearPlane						= 0.1f;
	inline constexpr	float							farPlane						= 100.0f;
}