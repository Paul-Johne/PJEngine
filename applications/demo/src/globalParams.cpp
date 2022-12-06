#include <globalParams.h>

pje::Context pje::context = {};

std::vector<pje::Vertex> pje::debugTriangle = {
	pje::Vertex({ 1.0f,  1.0f}, {5.0f, 0.0f, 0.0f}),
	pje::Vertex({ 0.5f,  0.5f}, {0.0f, 5.0f, 0.0f}),
	pje::Vertex({-0.5f,  0.5f}, {0.0f, 0.0f, 5.0f})
};

pje::PJBuffer pje::currentLoadForGPU = {};