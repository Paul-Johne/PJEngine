#include "pjeBuffers.h"

pje::engine::types::Vertex::Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, glm::uvec2 boneAttrib) : 
	m_pos(pos), m_normal(normal), m_uv(uv), m_boneAttrib(boneAttrib) {}

pje::engine::types::Vertex::~Vertex() {}

std::array<VkVertexInputAttributeDescription, 4> pje::engine::types::Vertex::getVulkanAttribDesc() {
	std::array<VkVertexInputAttributeDescription, 4> desc;

	desc[0].location	= 0;									// layout(location = 0)
	desc[0].binding		= 0;									// index of a VkVertexInputBindingDescription
	desc[0].format		= VkFormat::VK_FORMAT_R32G32B32_SFLOAT; // vec3 pos
	desc[0].offset		= offsetof(Vertex, m_pos);				// 0

	desc[1].location	= 1;									// layout(location = 1)
	desc[1].binding		= 0;
	desc[1].format		= VkFormat::VK_FORMAT_R32G32B32_SFLOAT;	// vec3 normal
	desc[1].offset		= offsetof(Vertex, m_normal);			// 3 * 4 Byte = 12

	desc[2].location	= 2;									// layout(location = 2)
	desc[2].binding		= 0;
	desc[2].format		= VkFormat::VK_FORMAT_R32G32_SFLOAT;	// vec2 uv
	desc[2].offset		= offsetof(Vertex, m_uv);				// 6 * 4 Byte = 24

	desc[3].location	= 3;									// layout(location = 3)
	desc[3].binding		= 0;
	desc[3].format		= VkFormat::VK_FORMAT_R32G32_UINT;		// uvec2 boneAttrib
	desc[3].offset		= offsetof(Vertex, m_boneAttrib);		// OR: 11 * 4 Bytes = 44

	return desc;
}

VkVertexInputBindingDescription pje::engine::types::Vertex::getVulkanBindingDesc() {
	VkVertexInputBindingDescription desc;
	desc.binding	= 0;
	desc.stride		= sizeof(Vertex);
	desc.inputRate	= VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	return desc;
}

/* ################################################################################### */

pje::engine::types::Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t offsetVertices, uint32_t offsetIndices) : 
	m_vertices(vertices), m_indices(indices), m_offsetPriorMeshesVertices(offsetVertices), m_offsetPriorMeshesIndices(offsetIndices) {}

pje::engine::types::Mesh::~Mesh() {}

/* ################################################################################### */

pje::engine::types::Primitive::Primitive() {}

pje::engine::types::Primitive::~Primitive() {}

/* ################################################################################### */

pje::engine::types::LSysPrimitive::LSysPrimitive() : Primitive() {}

pje::engine::types::LSysPrimitive::~LSysPrimitive() {}

/* ################################################################################### */

pje::engine::types::LSysObject::LSysObject() : m_matrices() {}

pje::engine::types::LSysObject::~LSysObject() {}

void pje::engine::types::LSysObject::placeObjectInWorld(const glm::vec3 translation, const float rotationDegreesY, const glm::vec3 scale) {
	const static glm::mat4 identityMat = glm::mat4(1.0f);

	glm::mat4 t = glm::translate(
		identityMat, translation
	);
	glm::mat4 r_y = glm::rotate(
		identityMat, glm::radians(rotationDegreesY), glm::vec3(0.0f, 1.0f, 0.0f)
	);
	glm::mat4 s = glm::scale(
		identityMat, scale
	);

	m_matrices.modelMatrix = t * r_y * s;
}

void pje::engine::types::LSysObject::placeCamera(const glm::vec3 posInWorld, const glm::vec3 focusCenter, const glm::vec3 cameraUp) {
	m_matrices.viewMatrix = glm::lookAt(posInWorld, focusCenter, cameraUp);
}

void pje::engine::types::LSysObject::setPerspective(const float fovY, const float aspectRatio, const float nearPlane, const float farPlane, API api) {
	m_matrices.projectionMatrix = glm::perspective(fovY, aspectRatio, nearPlane, farPlane);

	if (api == API::Vulkan)
		m_matrices.projectionMatrix[1][1] *= -1;
}

void pje::engine::types::LSysObject::updateMVP() {
	m_matrices.mvp = m_matrices.projectionMatrix * m_matrices.viewMatrix * m_matrices.modelMatrix;
}

void pje::engine::types::LSysObject::animWindBlow(const float deltaTime, const float blowStrength) {
	/* update m_bones to simulate an even changing wind power affecting the bones */
	const static glm::mat4 identityMat		= glm::mat4(1.0f);
	constexpr float PI						= 3.1415927f;
	const static float tiltUnitInRadians	= 20.0f * (PI / 180.0f)  ;

	glm::mat4 tiltMat = glm::rotate(
		identityMat, std::sinf(tiltUnitInRadians * deltaTime) * blowStrength, glm::vec3(0.0f, 0.0f, 1.0f)
	);

	// TODO(scene graph animation) !!
	for (auto& bone : m_bones) {
		bone.animationpose = bone.restpose * tiltMat;
	}
}

std::vector<glm::mat4> pje::engine::types::LSysObject::getBoneMatrices() const {
	std::vector<glm::mat4> res;

	for (const auto& bone : m_bones) {
		res.push_back(bone.animationpose * bone.restposeInv);
	}

	return res;
}