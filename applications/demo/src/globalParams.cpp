#include <globalParams.h>

pje::Context pje::context = {};

pje::Uniforms pje::uniforms = {
	glm::mat4(1.0f),
	glm::mat4(1.0f),
	glm::mat4(1.0f),
	glm::mat4(1.0f)
};

std::vector<pje::PJVertex> pje::debugVertices = {
	pje::PJVertex({ -0.5f, 0.0f, -0.5f}, {5.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({  0.5f, 0.0f, -0.5f}, {0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({ -0.5f, 0.0f,  0.5f}, {0.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({  0.5f, 0.0f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f})
};
std::vector<uint32_t> pje::debugIndices = {
	0, 2, 1,
	1, 2, 3
};

std::vector<pje::PJMesh> pje::debugMeshes = {
	pje::PJMesh(pje::debugVertices, pje::debugIndices)
};

pje::PJBuffer pje::stagingBuffer = {};
pje::PJBuffer pje::vertexBuffer = {};
pje::PJBuffer pje::indexBuffer = {};
pje::PJBuffer pje::uniformsBuffer = {};