#include <modelloader.h>

#include <iostream>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

pje::ModelLoader::ModelLoader() : m_models(), m_modelPaths(), m_active_models(0), m_centerModel(true), m_folderForModels("assets/models") {
	for (const auto& each : std::filesystem::directory_iterator(m_folderForModels)) {
		auto path = each.path().string();

		if (path.find(".fbx") != std::string::npos || path.find(".obj") != std::string::npos) {
			std::cout << "[DEBUG] 3D object found at: " << path << std::endl;
			loadModel(path, m_centerModel);
		}
	}
}

pje::ModelLoader::~ModelLoader() {}

void pje::ModelLoader::loadModel(const std::string& filename, bool centerModel) {
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	if (!pScene || !pScene->mRootNode || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		std::cout << "[Error] : " << importer.GetErrorString() << std::endl;
		throw std::runtime_error("Error at ModelLoader::loadModel!");
	}
	else {
		std::cout << "[DEBUG] Success at loading object from: " << filename << std::endl;
		m_modelPaths.push_back(filename);
		++m_active_models;
	}

	recurseNodes(pScene->mRootNode, pScene, centerModel);
}

void pje::ModelLoader::recurseNodes(aiNode* node, const aiScene* pScene, bool centerModel) {
	// extract all mesh data from this node
	for (uint32_t i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = pScene->mMeshes[node->mMeshes[i]];				// aiNode only knows index of aiMesh in aiScene
		m_models.push_back(translateMesh(mesh, pScene, centerModel));
	}

	// go through each child's node (and all its children) in sequence
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		recurseNodes(node->mChildren[i], pScene, centerModel);			// aiNode's children only know index of aiMesh in aiScene
	}
}

pje::PJMesh pje::ModelLoader::translateMesh(aiMesh* mesh, const aiScene* pScene, bool centerModel) {
	std::vector<PJVertex> vertices;
	std::vector<uint32_t> indices;

	// copy VBO
	for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
		PJVertex currentVertex(
			{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },	// glm::vec3 m_position
			{ 1.0f, 1.0f, 1.0f },													// glm::vec3 m_color

			// TODO (Check if valid)
			{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }		// glm::vec3 m_normal
		);

		vertices.push_back(currentVertex);
	}

	if (centerModel) {
		glm::vec3 avgPos{ 0.0f, 0.0f, 0.0f };

		for (const auto& v : vertices) {
			avgPos += v.m_position;
		}
		avgPos /= vertices.size();

		for (auto& v : vertices) {
			v.m_position -= avgPos;
		}
	}

	// copy IBO
	for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	return PJMesh(vertices, indices);
}