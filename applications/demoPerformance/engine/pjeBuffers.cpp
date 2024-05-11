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

pje::engine::types::Primitive::Primitive() {}

pje::engine::types::Primitive::~Primitive() {}

/* ################################################################################### */

pje::engine::types::Mesh::Mesh() {}

pje::engine::types::Mesh::~Mesh() {}

/* ################################################################################### */

pje::engine::types::LSysObject::LSysObject() {}

pje::engine::types::LSysObject::~LSysObject() {}