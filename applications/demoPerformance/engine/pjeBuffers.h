/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>			// fixed size integer
	#include <string>			// std::string
	#include <array>			// std::array
	#include <vector>			// std::vector
	#include <memory>			// std::<smartPointer>

	#include <vulkan/vulkan.h>	// Vulkan
	#include <glm/glm.hpp>		// glm
	#include <stb_image.h>		// stb

/* PJE Types - holding data for both Vulkan and OpenGL */
namespace pje::engine::types {

	/* Vertex - Layout for Vulkan and OpenGL */
	class Vertex {
	public:
		glm::vec3	m_pos;
		glm::vec3	m_normal;
		glm::vec2	m_uv;
		glm::uvec2	m_boneAttrib;

		Vertex() = delete;
		Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, glm::uvec2 boneAttrib = glm::uvec2(0, 0));
		~Vertex();

		/* Vulkan-specific */
		static std::array<VkVertexInputAttributeDescription, 4> getVulkanAttribDesc();
		static VkVertexInputBindingDescription getVulkanBindingDesc();
	};

	/* Texture - 1 Primitive <-> 1 Texture */
	struct Texture {
		std::vector<unsigned char>	uncompressedTexture;
		std::string					name;
		int							width;
		int							height;
		int							channels;
	};

	/* MVPMatrices - used to place model in scene */
	struct MVPMatrices {
		glm::mat4 mvp;				// object space -> screen space
		glm::mat4 modelMatrix;		// object space -> world space
		glm::mat4 viewMatrix;		// world space	-> camera space
		glm::mat4 projectionMatrix;	// camera space -> screen space
	};

	/* BoneRefs - shader ressource to reference BoneMatrices::animationpose */
	struct BoneRefs {
		uint32_t	boneId;
		float		weight;
	};

	/* BoneMatrices - shared data container for multiple Primitive(s) */
	struct BoneMatrices {
		/*
		*	- transformation matrix	(1)				(m_restpose)		: local/primitive/bone space -> object space
		*	- "local view transformation" matrix	(m_restposeInv)		: object space -> local/primitive/bone space
		*	- transformation matrix (2)				(m_animationpose)	: bone space -> object space
		*/
		glm::mat4 restpose;			// O_i		= O_(i-1)  * T_y_i    * R_i
		glm::mat4 restposeInv;		// O_i^-1	= R_i^-1   * T_y_i^-1 * O_(i-1)^-1
		glm::mat4 animationpose;	// O'_i		= O'_(i-1) * <transformation matrix>
	};

	/* Mesh - 1 Primitive <-> n Mesh(es) */
	class Mesh {
	public:
		std::vector<Vertex>				m_vertices;
		std::vector<uint32_t>			m_indices;
		uint32_t						m_offsetPriorMeshVertices;
		uint32_t						m_offsetPriorMeshIndices;

		Mesh();
		~Mesh();
	};

	/* Primitive - 1 LSysObject <-> n Primitive(s)*/
	class Primitive {
	public:
		std::vector<Mesh>				m_meshes;
		Texture							m_texture;							// texture ressource
		std::shared_ptr<BoneMatrices>	boneMatrices;						// 1 Primitive <-> 1 Bone
		uint32_t						m_offsetPriorPrimitiveVertices;
		uint32_t						m_offsetPriorPrimitiveIndices;

		Primitive();
		~Primitive();
	};

	/* LSysObject - created by TurtleInterpreter | represents logical renderable */
	class LSysObject {
	public:
		MVPMatrices				m_matrices;		// object space -> world/camera/screen space
		std::vector<Primitive>	m_primitives;	// primitives are in local space

		LSysObject();
		~LSysObject();
	};
}