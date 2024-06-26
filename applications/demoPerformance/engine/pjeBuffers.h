/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>							// fixed size integer
	#include <string>							// std::string
	#include <array>							// std::array
	#include <vector>							// std::vector
	#include <memory>							// std::<smartPointer>
	#include <chrono>							// (animation) time measurement
	#include <cmath>							// (animation) math functions

	#include <vulkan/vulkan.h>					// Vulkan
	#include <glm/glm.hpp>						// glm types
	#include <glm/gtc/matrix_transform.hpp>		// glm matrix operations
	#include <stb_image.h>						// stb

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
		std::string					name;
		std::vector<unsigned char>	uncompressedTexture;

		int							width;
		int							height;
		int							channels;

		/* width * height * channels */
		size_t						size;
	};

	/* MVPMatrices - used to place model in scene (init => identity matrices) */
	struct MVPMatrices {
		glm::mat4 mvp				= glm::mat4(1.0f);	// object space -> screen space
		glm::mat4 modelMatrix		= glm::mat4(1.0f);	// object space -> world space
		glm::mat4 viewMatrix		= glm::mat4(1.0f);	// world space	-> camera space
		glm::mat4 projectionMatrix	= glm::mat4(1.0f);	// camera space -> screen space
	};

	/* BoneRef - shader ressource to reference a BoneMatrix inside of shader */
	struct BoneRef {
		uint32_t	boneId;
		float		weight;
	};

	/* Bone - shared data container for multiple Primitive(s) */
	struct Bone {
		/* BoneMatrix (for shaders) := animationpose * restposeInv | [ O'_i * O_i^-1 ]
		*	- transformation matrix	(1)				(m_restpose)		: local/primitive/bone space -> object space
		*	- "local view transformation" matrix	(m_restposeInv)		: object space -> local/primitive/bone space
		*	- transformation matrix (2)				(m_animationpose)	: bone space -> object space
		*/
		glm::mat4 restpose;			// O_i		= O_(i-1)  * T_y_i    * R_i
		glm::mat4 restposeInv;		// O_i^-1	= R_i^-1   * T_y_i^-1 * O_(i-1)^-1
		glm::mat4 animationpose;	// O'_i		= O_i      * <transformation matrix>
	};

	/* Mesh - 1 Primitive <-> n Mesh(es) */
	class Mesh {
	public:
		std::vector<Vertex>				m_vertices;
		std::vector<uint32_t>			m_indices;
		uint32_t						m_offsetPriorMeshesVertices = 0;	// helper during sourceloader
		uint32_t						m_offsetPriorMeshesIndices = 0;		// helper during sourceloader

		Mesh() = delete;
		Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t offsetVertices, uint32_t offsetIndices);
		~Mesh();
	};

	/* Primitive - 1 LSysObject <-> n Primitive(s)*/
	class Primitive {
	public:
		std::string			m_identifier;	// unique identifier for a primitive set of some TurtleInterpreter
		std::vector<Mesh>	m_meshes;
		Texture				m_texture;		// texture ressource

		Primitive();
		virtual ~Primitive();
	};

	/* LSysPrimitive - modified Primitives | logical component of LSysObject */
	class LSysPrimitive final : public Primitive {
	public:
		uint32_t						m_offsetPriorPrimitivesVertices	= 0;	// helper: address this primitive's vertices count
		uint32_t						m_offsetPriorPrimitivesIndices	= 0;	// helper: address this primitive's indices count

		LSysPrimitive();
		~LSysPrimitive();
	};

	/* LSysObject - created by TurtleInterpreter | represents logical renderable */
	class LSysObject {
	public:
		enum class API { Vulkan, OpenGL };				// Y Axis -> +infinite ==> Vulkan down & OpenGL up

		std::vector<LSysPrimitive>	m_objectPrimitives; // primitives placed in object space
		MVPMatrices					m_matrices;			// object space -> world/camera/screen space

		Texture						m_choosenTexture;	// PROJECT LIMITATION: same texture map for all primitives
		std::vector<Bone>			m_bones;			// PROJECT LIMITATION: 1 boneMatrix <-> 1+ LSysPrimitive   !!!
		std::vector<BoneRef>		m_boneRefs;			// PROJECT LIMITATION: 1 boneRef	<-> 1  LSysPrimitive

		LSysObject();
		~LSysObject();

		/* scene logic */
		void placeObjectInWorld(const glm::vec3 translation, const float rotationDegreesY, const glm::vec3 scale);
		void placeCamera(const glm::vec3 posInWorld, const glm::vec3 focusCenter, const glm::vec3 cameraUp);
		void setPerspective(const float fovY, const float aspectRatio, const float nearPlane, const float farPlane, API api);
		void updateMVP();

		/* animation logic => m_bones manipulation */
		void animWindBlow(const float deltaTime, const float blowStrength = 1.0f);
		std::vector<glm::mat4> getBoneMatrices() const;
	};
}