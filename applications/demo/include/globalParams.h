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
		size_t							choosenPhysicalDevice = 0;
		VkDevice						logicalDevice;

		VkQueue	queueForPrototyping;

		VkSwapchainKHR						swapchain = VK_NULL_HANDLE;
		uint32_t							numberOfImagesInSwapchain = 0;
		std::unique_ptr<VkImageView[]>		imageViews;
		std::unique_ptr<VkFramebuffer[]>	framebuffers;

		VkShaderModule	shaderModuleBasicVert;
		VkShaderModule	shaderModuleBasicFrag;
		std::vector<VkPipelineShaderStageCreateInfo>	shaderStageInfos;

		VkPipelineLayout	pipelineLayout;
		VkRenderPass		renderPass;
		VkPipeline			pipeline;

		VkCommandPool						commandPool;
		std::unique_ptr<VkCommandBuffer[]>	commandBuffers;

		const VkFormat		outputFormat = VK_FORMAT_B8G8R8A8_UNORM;				// civ => VkFormat 44
		const VkClearValue	clearValueDefault = { 0.588f, 0.294f, 0.0f, 1.0f };		// DEFAULT BACKGROUND (RGB) for all rendered images

		VkSemaphore	semaphoreSwapchainImageReceived;
		VkSemaphore	semaphoreRenderingFinished;
		VkFence		fenceRenderFinished;											// 2 States: signaled, unsignaled

		GLFWwindow* window;
		uint32_t	windowWidth = 1280;
		uint32_t	windowHeight = 720;
	};

	extern Context context;
}