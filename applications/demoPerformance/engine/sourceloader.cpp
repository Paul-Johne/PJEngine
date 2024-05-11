#include "sourceloader.h"

#define DEFAULT_ASSIMP_FLAGS aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices

pje::engine::Sourceloader::Sourceloader() : m_primitives(), 
											m_primitivePaths(), 
											m_activePrimitivesCount(0), 
											m_textures(), 
											m_centerPrimitive(false), 
											m_sourceFolder("assets/primitives") {
	for (const auto& each : std::filesystem::directory_iterator(m_sourceFolder)) {
		auto path = each.path().string();

		if (path.find(".fbx") != std::string::npos) {
			std::cout << "[PJE] \t.fbx file found at: " << path << std::endl;
			loadPrimitive(path, DEFAULT_ASSIMP_FLAGS, m_centerPrimitive);
		}
	}
}

pje::engine::Sourceloader::~Sourceloader() {}

void pje::engine::Sourceloader::loadPrimitive(const std::string& filename, unsigned int flags, bool centerPrimitive) {
	Assimp::Importer importer;
	//TODO
}

glm::mat4 pje::engine::Sourceloader::matrix4x4Assimp2glm(const aiMatrix4x4& assimpMatrix) {
	glm::mat4 glmMatrix;

	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			glmMatrix[column][row] = assimpMatrix[row][column];
		}
	}

	return glmMatrix;
}