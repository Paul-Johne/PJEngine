/* Ignores header if not needed after #include */
	#pragma once

	#include <memory>
	#include <vector>
	#include <array>

	#include <vulkan/vulkan.h>
	#include <GLFW/glfw3.h>
	#include <glm/glm.hpp>
	#include <imgui.h>

/* Collection of global parameters */
namespace pje {

	/* Blueprint */
	struct Context {
		VkResult	result;
		const char* appName = "PJEngine";

		VkInstance						vulkanInstance;
		VkSurfaceKHR					surface;
		std::vector<VkPhysicalDevice>	physicalDevices;
		VkDevice						logicalDevice;

		VkPhysicalDeviceType			preferredPhysicalDeviceType = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		std::vector<VkQueueFlagBits>	neededFamilyQueueAttributes = { VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT };
		uint32_t						neededSurfaceImages = 2;
		VkPresentModeKHR				neededPresentationMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;

		uint32_t						choosenPhysicalDevice;						// dynamically by selectGPU()
		uint32_t						choosenQueueFamily;							// dynamically by selectGPU()

		VkQueue	queueForPrototyping;

		VkSwapchainKHR						swapchain = VK_NULL_HANDLE;
		uint32_t							numberOfImagesInSwapchain = 0;
		std::unique_ptr<VkImageView[]>		imageViews;
		std::unique_ptr<VkFramebuffer[]>	framebuffers;

		VkShaderModule									shaderModuleBasicVert;
		VkShaderModule									shaderModuleBasicFrag;
		std::vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;

		VkPipelineLayout	pipelineLayout;
		VkRenderPass		renderPass;
		VkPipeline			pipeline;

		VkCommandPool						commandPool;
		std::unique_ptr<VkCommandBuffer[]>	commandBuffers;

		const VkFormat		outputFormat = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;		// VkFormat 44
		const VkClearValue	clearValueDefault = { 0.588f, 0.294f, 0.0f, 1.0f };		// DEFAULT BACKGROUND

		VkSemaphore	semaphoreSwapchainImageReceived;
		VkSemaphore	semaphoreRenderingFinished;
		VkFence		fenceRenderFinished;											// 2 States: signaled, unsignaled

		GLFWwindow* window;
		bool		isWindowMinimized = false;
		uint32_t	windowWidth = 1280;
		uint32_t	windowHeight = 720;
	};
	extern Context context;

	class Vertex {
	public:
		glm::vec2 position;
		glm::vec3 color;

		Vertex(glm::vec2 position, glm::vec3 color) : 
			position(position), color(color) {}									// initialization list
	
		static VkVertexInputBindingDescription getInputBindingDesc() {
			VkVertexInputBindingDescription desc;
			desc.binding = 0;													// index of a VkVertexInputBindingDescription
			desc.stride = sizeof(Vertex);										// byte size of one Vertex
			desc.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;	// per Instance | per Vertex

			return desc;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getInputAttributeDesc() {	// array size equals amount of members in pje::Vertex
			std::array<VkVertexInputAttributeDescription, 2> attributes;
			
			attributes[0].location = 0;										// Vertex Input Buffer
			attributes[0].binding = 0;										// index of a VkVertexInputBindingDescription
			attributes[0].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;		// vec2
			attributes[0].offset = offsetof(Vertex, position);				// 0

			attributes[1].location = 1;
			attributes[1].binding = 0;
			attributes[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;	// vec3
			attributes[1].offset = offsetof(Vertex, color);					// 2 * 4 Byte = 8

			return attributes;
		}
	};
	extern std::vector<Vertex> debugTriangle;

	struct PJBuffer {
		VkBuffer		vertexVkBuffer;
		VkDeviceMemory	vertexDeviceMemory;

		const VkMemoryPropertyFlags flags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	};
	extern PJBuffer currentLoadForGPU;
}