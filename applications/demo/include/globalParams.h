/* Ignores header if not needed after #include */
	#pragma once

	#include <memory>
	#include <vector>
	#include <array>
	#include <chrono>

	#include <vulkan/vulkan.h>
	#include <GLFW/glfw3.h>
	#include <glm/glm.hpp>
	#include <assimp/texture.h>
	#include <imgui.h>

	#include <config.h>

/* Collection of global parameters */
namespace pje {

	/* #### CONTEXT #### */
	struct Context {
		VkResult											result;
		std::chrono::time_point<std::chrono::steady_clock>	startTimePoint;

		VkInstance											vulkanInstance					= VK_NULL_HANDLE;
		VkSurfaceKHR										surface							= VK_NULL_HANDLE;
		std::vector<VkPhysicalDevice>						physicalDevices;
		VkDevice											logicalDevice					= VK_NULL_HANDLE;

		// dynamically choosen by selectGPU()
		uint32_t											choosenPhysicalDevice;						
		uint32_t											choosenQueueFamily;

		VkQueue												queueForPrototyping				= VK_NULL_HANDLE;

		VkSwapchainKHR										swapchain						= VK_NULL_HANDLE;
		uint32_t											numberOfImagesInSwapchain		= 0;
		std::unique_ptr<VkImageView[]>						swapchainImageViews;
		std::unique_ptr<VkFramebuffer[]>					swapchainFramebuffers;

		VkDeviceMemory										msaaImageMemory					= VK_NULL_HANDLE;
		std::unique_ptr<VkImage>							msaaImage;
		std::unique_ptr<VkImageView>						msaaImageView;
		VkDeviceMemory										depthImageMemory				= VK_NULL_HANDLE;
		std::unique_ptr<VkImage>							depthImage;
		std::unique_ptr<VkImageView>						depthImageView;

		VkShaderModule										shaderModuleBasicVert			= VK_NULL_HANDLE;
		VkShaderModule										shaderModuleBasicFrag			= VK_NULL_HANDLE;
		std::vector<VkPipelineShaderStageCreateInfo>		shaderStageInfos;

		VkRenderPass										renderPass						= VK_NULL_HANDLE;
		VkPipelineLayout									pipelineLayout					= VK_NULL_HANDLE;
		VkPipeline											pipeline						= VK_NULL_HANDLE;

		VkCommandPool										commandPool						= VK_NULL_HANDLE;
		std::unique_ptr<VkCommandBuffer[]>					commandBuffers;

		VkDescriptorSetLayout								descriptorSetLayout				= VK_NULL_HANDLE;
		VkDescriptorPool									descriptorPool					= VK_NULL_HANDLE;
		VkDescriptorSet										descriptorSet					= VK_NULL_HANDLE;

		// 2 States: signaled, unsignaled
		VkSemaphore											semaphoreSwapchainImageReceived	= VK_NULL_HANDLE;
		VkSemaphore											semaphoreRenderingFinished		= VK_NULL_HANDLE;
		VkFence												fenceRenderFinished				= VK_NULL_HANDLE;
		VkFence												fenceCopiedBuffer				= VK_NULL_HANDLE;

		GLFWwindow*											window;
		bool												isWindowMinimized				= false;
		uint32_t											windowWidth						= config::initWindowWidth;
		uint32_t											windowHeight					= config::initWindowHeight;
	};
	extern Context context;

	/* #### UNIFORMS #### */
	struct Uniforms {
		glm::mat4 mvp;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
	};
	extern Uniforms uniforms;

	/* #### STORAGE BUFFER #### */
	struct BoneRef {
		uint32_t boneId;
		float weight;
	};
	extern std::vector<BoneRef> boneRefs;
	// extern std::vector<glm::mat4> boneMatrices;

	/* #### VERTEX #### */
	class PJVertex {
	public:
		glm::vec3 m_position;
		glm::vec3 m_color;
		glm::vec3 m_normal;
		glm::vec2 m_boneRange;

		PJVertex() = delete;
		PJVertex(glm::vec3 position, glm::vec3 color, glm::vec3 normal, glm::uvec2 boneRange = glm::uvec2(0, 0)) :
			m_position(position), m_color(color), m_normal(normal), m_boneRange(boneRange) {}	// initialization list
		
		~PJVertex()
			{}
	
		/* integrated into vulkan pipeline */
		static VkVertexInputBindingDescription getInputBindingDesc() {
			VkVertexInputBindingDescription desc;
			desc.binding = 0;													// index of a VkVertexInputBindingDescription
			desc.stride = sizeof(PJVertex);										// byte size of one Vertex
			desc.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;	// per Instance | per Vertex

			return desc;
		}

		/* integrated into vulkan pipeline */
		/* array size equals amount of members in pje::PJVertex */
		static std::array<VkVertexInputAttributeDescription, 4> getInputAttributeDesc() {
			std::array<VkVertexInputAttributeDescription, 4> attributes;
			
			attributes[0].location = 0;											// Vertex Input Buffer => layout(location = 0)
			attributes[0].binding = 0;											// index of a VkVertexInputBindingDescription
			attributes[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;		// vec3
			attributes[0].offset = offsetof(PJVertex, m_position);				// 0

			attributes[1].location = 1;											// Vertex Input Buffer => layout(location = 1)
			attributes[1].binding = 0;											// index of a VkVertexInputBindingDescription
			attributes[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;		// vec3
			attributes[1].offset = offsetof(PJVertex, m_color);					// OR: 3 * 4 Byte = 12

			attributes[2].location = 2;
			attributes[2].binding = 0;
			attributes[2].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
			attributes[2].offset = offsetof(PJVertex, m_normal);				// OR: 6 * 4 Byte = 24

			attributes[3].location = 3;
			attributes[3].binding = 0;
			attributes[3].format = VkFormat::VK_FORMAT_R32G32_UINT;
			attributes[3].offset = offsetof(PJVertex, m_boneRange);				// OR: 9 * 4 Byte = 36

			return attributes;
		}
	};
	extern std::vector<PJVertex> debugVertices;
	extern std::vector<uint32_t> debugIndices;

	/* #### Mesh #### */
	class PJMesh {
	public:
		using vertexType = PJVertex;
		using indexType = uint32_t;

		std::vector<vertexType>	m_vertices;
		std::vector<indexType>	m_indices;
		indexType				m_offsetVertices;		// to prior meshes
		indexType				m_offsetIndices;		// to prior meshes

		/* m_vertices copies data of vertices */
		PJMesh(const std::vector<PJVertex>& vertices, const std::vector<uint32_t>& indices, uint32_t offsetVertices, uint32_t offsetIndices) :
			m_vertices(vertices), m_indices(indices) , m_offsetVertices(offsetVertices), m_offsetIndices(offsetIndices) {}

		~PJMesh()
			{}
	};
	extern std::vector<PJMesh>	debugMesh;

	/* #### Model #### */
	class PJModel {
	public:
		std::vector<PJMesh>				meshes;
		std::vector<const aiTexture*>	textures;
		std::string						modelPath;
		bool							centered;
	};
	extern std::vector<pje::PJModel>	loadedModels;

	/* #### PJBUFFER #### */
	struct PJBuffer {
		VkBuffer				buffer;
		VkDeviceSize			size;

		VkDeviceMemory			deviceMemory;
		VkMemoryPropertyFlags	flags;
	};
	extern PJBuffer stagingBuffer;
	extern PJBuffer vertexBuffer;
	extern PJBuffer indexBuffer;

	extern PJBuffer uniformsBuffer;
	extern PJBuffer storeBoneRefs;
	extern PJBuffer storeBoneMatrices;
}