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
	#include "../engine/argsParser.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	/* DescriptorSetElement - needed at RendererVK::setDescriptorSet to define shader resources */
	struct DescriptorSetElementVK {
		VkDescriptorType		type;
		uint32_t				arraySize;
		VkShaderStageFlagBits	flagsMask;
	};

	/* BufferVK - container for handling VBOs/IBOs/Uniform Buffers/Storage Buffers */
	struct BufferVK {
		VkDevice hostDevice		= VK_NULL_HANDLE;

		VkBuffer buffer			= VK_NULL_HANDLE;
		VkDeviceMemory memory	= VK_NULL_HANDLE;
		VkDeviceSize size		= 0;

		~BufferVK() {
			if (hostDevice != VK_NULL_HANDLE) {
				if (memory != VK_NULL_HANDLE)
					vkFreeMemory(hostDevice, memory, nullptr);
				if (buffer != VK_NULL_HANDLE)
					vkDestroyBuffer(hostDevice, buffer, nullptr);
				hostDevice = VK_NULL_HANDLE;
			}
		}
	};

	/* ImageVK - container for handling images/rendertargets */
	struct ImageVK {
		VkDevice hostDevice		= VK_NULL_HANDLE;

		VkFormat format			= VkFormat::VK_FORMAT_UNDEFINED;
		VkImage image			= VK_NULL_HANDLE;
		VkDeviceMemory memory	= VK_NULL_HANDLE;
		VkImageView imageView	= VK_NULL_HANDLE;
		unsigned int mipCount	= 0;

		~ImageVK() {
			if (hostDevice != VK_NULL_HANDLE) {
				if (imageView != VK_NULL_HANDLE)
					vkDestroyImageView(hostDevice, imageView, nullptr);
				if (memory != VK_NULL_HANDLE)
					vkFreeMemory(hostDevice, memory, nullptr);
				if (image != VK_NULL_HANDLE)
					vkDestroyImage(hostDevice, image, nullptr);
				hostDevice = VK_NULL_HANDLE;
			}
		}
	};

	/* ContextVK - holds all manually managed Vulkan resources */
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
		VkSampler										texSampler				= VK_NULL_HANDLE;

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

		BufferVK										buffStaging;
		BufferVK										buffVertices;
		BufferVK										buffIndices;
	};

	/* RendererVK - Vulkan powered renderer for demoPerformance */
	class RendererVK final {
	public:
		enum class TextureType	{ Albedo };
		enum class BufferType	{ UniformMVP, StorageBoneRefs, StorageBones };

		ImageVK		m_texAlbedo;
		BufferVK	m_buffUniformMVP;
		BufferVK	m_buffStorageBoneRefs;
		BufferVK	m_buffStorageBones;

		RendererVK() = delete;
		RendererVK(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable);
		~RendererVK();

		/** Methods for app.cpp **/
		/* 1/4: Uploading */
		void uploadRenderable(const pje::engine::types::LSysObject& renderable);
		void uploadTextureOf(const pje::engine::types::LSysObject& renderable, bool genMipmaps, TextureType type);
		void uploadBuffer(const pje::engine::types::LSysObject& renderable, BufferVK& m_VarRaw, BufferType type);
		/* 2/4: Binding shader resources */
		void bindToShader(const BufferVK& buffer, uint32_t dstBinding, VkDescriptorType descType);
		void bindToShader(const ImageVK& image, uint32_t dstBinding, VkDescriptorType descType);
		/* 3/4: Rendering */
		void renderIn(GLFWwindow* window, const pje::engine::types::LSysObject& renderable);
		/* 4/4: Updating */
		void updateBuffer(const pje::engine::types::LSysObject& renderable, BufferVK& m_Var, BufferType type);

	private:
		enum class RequestLevel		{ Instance, Device };
		enum class AnisotropyLevel	{ Disabled, TWOx, FOURx, EIGHTx, SIXTEENx };

		ContextVK				m_context;
		uint16_t				m_renderWidth;
		uint16_t				m_renderHeight;
		bool					m_windowIconified;
		bool					m_vsync;
		AnisotropyLevel			m_anisotropyLevel;
		VkSampleCountFlagBits	m_msaaFactor;
		uint8_t					m_instanceCount;

		std::string getApiVersion();
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
		void setTexSampler();
		void setRenderpass();
		void setRendertarget(ImageVK& rt, VkExtent3D imgSize, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask);
		void setSwapchain(VkExtent2D imgSize);
		void setFramebuffers(uint32_t renderPassAttachmentCount);
		void setFence(VkFence& fence, bool isSignaled);
		void setSemaphore(VkSemaphore& sem);
		void setCommandPool();
		void allocateCommandbufferOf(const VkCommandPool& cbPool, uint32_t cbCount, VkCommandBuffer* pCommandBuffers);
		void buildPipeline();
		bool requestLayer(RequestLevel level, std::string layerName);
		bool requestExtension(RequestLevel level, std::string extensionName);
		void requestGlfwInstanceExtensions();
		std::vector<char> loadShader(const std::string& filename);
		VkDeviceMemory allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags flagMask);
		VkBuffer allocateBuffer(VkDeviceSize requiredSize, VkBufferUsageFlags usage);
		void prepareStaging(VkDeviceSize requiredSize);
		void copyStagedBuffer(VkBuffer dst, const VkDeviceSize offsetInDst, const VkDeviceSize dataInfo);
		void copyStagedBuffer(VkImage dst, const pje::engine::types::Texture texInfo);
		void generateMipmaps(ImageVK& uploadedTexture, unsigned int baseTexWidth, unsigned int baseTexHeight);
		void recordCbRenderingFor(const pje::engine::types::LSysObject& renderable, uint32_t imgIndex);
	};
}