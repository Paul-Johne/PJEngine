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

		VkInstance		vulkanInstance;
		VkSurfaceKHR	surface;
		VkDevice		logicalDevice;

		VkQueue	queueForPrototyping;

		VkSwapchainKHR swapchain;
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
		const VkClearValue	clearValueDefault = { 0.0f, 0.0f, 1.0f, 0.5f };			// DEFAULT BACKGROUND (RGB) for all rendered images

		VkSemaphore	semaphoreSwapchainImageReceived;
		VkSemaphore	semaphoreRenderingFinished;
		VkFence		fenceRenderFinished;											// 2 States: signaled, unsignaled

		GLFWwindow* window;
		const uint32_t	WINDOW_WIDTH = 1280;
		const uint32_t	WINDOW_HEIGHT = 720;
	};

	extern Context context;
}