#include <globalParams.h>

pje::Context pje::context = {};

std::vector<pje::Vertex> pje::debugVertices = {
	pje::Vertex({ -0.5f,  -0.5f, 0.5f}, {5.0f, 0.0f, 0.0f}),
	pje::Vertex({  0.5f,   0.5f, 0.5f}, {0.0f, 5.0f, 0.0f}),
	pje::Vertex({ -0.5f,   0.5f, 0.5f}, {0.0f, 0.0f, 5.0f}),
	pje::Vertex({  0.5f,  -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f})
};
std::vector<uint32_t> pje::debugIndices = {
	0, 1, 2,
	0, 3, 1
};

pje::PJBuffer pje::stagingBuffer = {};
pje::PJBuffer pje::vertexBuffer = {};
pje::PJBuffer pje::indexBuffer = {};
pje::PJBuffer pje::mvpUniformBuffer = {};