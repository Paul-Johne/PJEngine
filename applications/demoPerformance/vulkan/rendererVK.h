/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>				// fixed size integer
	#include <string>				// std::string
	#include <array>				// std::array
	#include <vector>				// std::vector
	#include <algorithm>			// classic functions for ranges
	#include <limits>				// std::numeric_limits
	#include <fstream>				// read from files
	#include <iostream>				// i/o stream

	#include <vulkan/vulkan.h>
	#include <GLFW/glfw3.h>

/* Project Files */
	#include "../engine/rendererInterface.h"
	#include "../engine/argsParser.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	/* DescriptorSetElement - needed at RendererVK::setDescriptorSet to define shader ressources */
	struct DescriptorSetElementVK {
		VkDescriptorType		type;
		uint32_t				arraySize;
		VkShaderStageFlagBits	flagsMask;
	};

	/* ImageVK - container for handling images/rendertargets */
	struct ImageVK {
		VkFormat format			= VkFormat::VK_FORMAT_UNDEFINED;
		VkImage image			= VK_NULL_HANDLE;
		VkDeviceMemory memory	= VK_NULL_HANDLE;
		VkImageView imageView	= VK_NULL_HANDLE;
		unsigned int mipCount	= 0;
	};

	/* ContextVK - holds all manually managed Vulkan ressources */
	struct ContextVK {
		VkInstance										instance				= VK_NULL_HANDLE;
		std::vector<std::string>						instanceLayers;
		std::vector<std::string>						instanceExtensions;

		VkSurfaceKHR									surface					= VK_NULL_HANDLE;
		uint32_t										surfaceImageCount		= 2;
		VkSurfaceFormatKHR								surfaceFormat			= { VkFormat::VK_FORMAT_UNDEFINED, VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR };

		VkPhysicalDevice								gpu						= VK_NULL_HANDLE;
		VkDevice										device					= VK_NULL_HANDLE;
		std::vector<std::string>						deviceExtensions;
		VkQueue											deviceQueue				= VK_NULL_HANDLE;
		uint32_t										deviceQueueFamilyIndex  = std::numeric_limits<uint32_t>::max();

		VkShaderModule									vertexModule			= VK_NULL_HANDLE;
		VkShaderModule									fragmentModule			= VK_NULL_HANDLE;
		std::vector<VkPipelineShaderStageCreateInfo>	shaderProgram;

		VkDescriptorSetLayout							descriptorSetLayout		= VK_NULL_HANDLE;
		VkDescriptorPool								descriptorPool			= VK_NULL_HANDLE;
		VkDescriptorSet									descriptorSet			= VK_NULL_HANDLE;

		VkRenderPass									renderPass				= VK_NULL_HANDLE;
		VkPipelineLayout								pipelineLayout			= VK_NULL_HANDLE;
		VkPipeline										pipeline				= VK_NULL_HANDLE;

		VkSwapchainKHR									swapchain				= VK_NULL_HANDLE;
		std::vector<VkImageView>						swapchainImgViews;
		ImageVK											rtMsaa;
		ImageVK											rtDepth;
		std::vector<VkFramebuffer>						renderPassFramebuffers;

		VkFence											fenceSetupTask			= VK_NULL_HANDLE;
		VkFence											fenceImgRendered		= VK_NULL_HANDLE;
		VkSemaphore										semSwapImgReceived		= VK_NULL_HANDLE;
		VkSemaphore										semImgRendered			= VK_NULL_HANDLE;

		VkCommandPool									commandPool				= VK_NULL_HANDLE;
		std::vector<VkCommandBuffer>					cbsRendering;
		VkCommandBuffer									cbStaging				= VK_NULL_HANDLE;
		VkCommandBuffer									cbMipmapping			= VK_NULL_HANDLE;
	};

	/* RendererVK - Vulkan powered renderer for demoPerformance */
	class RendererVK final : public pje::renderer::RendererInterface {
	public:
		RendererVK() = delete;
		RendererVK(const pje::engine::ArgsParser& parser, GLFWwindow* const window, pje::engine::types::LSysObject renderable);
		~RendererVK();
		std::string getApiVersion();

	private:
		enum class RequestLevel { Instance, Device };
		enum class AnisotropyLevel { Disabled, TWOx, FOURx, EIGHTx, SIXTEENx };

		ContextVK				m_context;
		uint16_t				m_renderWidth;
		uint16_t				m_renderHeight;
		bool					m_vsync;
		AnisotropyLevel			m_anisotropyLevel;
		VkSampleCountFlagBits	m_msaaFactor;

		void setInstance();
		void setSurface(GLFWwindow* window);
		void setPhysicalDevice(
			VkPhysicalDeviceType					gpuType,
			VkQueueFlags							requiredQueueAttributes,
			uint32_t								requiredSurfaceImages,
			const std::vector<VkSurfaceFormatKHR>&	formatOptions
		);
		void setSurfaceFormatForPhysicalDevice(VkSurfaceFormatKHR format);
		void setDeviceAndQueue(VkQueueFlags requiredQueueAttributes);
		void setShaderModule(VkShaderModule& shaderModule, const std::vector<char>& shaderCode);
		void setShaderProgram(std::string shaderName);
		void setDescriptorSet(const std::vector<DescriptorSetElementVK>& elements);
		void setRenderpass();
		void setRendertarget(ImageVK& rt, VkExtent3D imgSize, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask);
		void setSwapchain(VkExtent2D imgSize);
		void setFramebuffers(uint32_t renderPassAttachmentCount);
		void setFence(VkFence& fence, bool isSignaled);
		void setSemaphore(VkSemaphore& sem);
		void setCommandPool();
		void setCommandbuffer();
		void buildPipeline();
		bool requestLayer(RequestLevel level, std::string layerName);
		bool requestExtension(RequestLevel level, std::string extensionName);
		void requestGlfwInstanceExtensions();
		std::vector<char> loadShader(const std::string& filename);
		VkDeviceMemory allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags flagMask);
	};
}