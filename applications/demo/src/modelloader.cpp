#include <modelloader.h>

#include <iostream>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

pje::ModelLoader::ModelLoader() : m_models(), m_modelPaths(), m_activeModels(0), m_centerModel(false), m_folderForModels("assets/models") {
	for (const auto& each : std::filesystem::directory_iterator(m_folderForModels)) {
		auto path = each.path().string();

		if (path.find(".fbx") != std::string::npos || path.find(".obj") != std::string::npos) {
			std::cout << "[DEBUG] 3D object found at: " << path << std::endl;
			m_models.push_back(loadModel(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices, m_centerModel));
		}
	}
}

pje::ModelLoader::~ModelLoader() {}

pje::PJModel pje::ModelLoader::loadModel(const std::string& filename, unsigned int pFlags, bool centerModel) {
	Assimp::Importer	importer;
	uint32_t			offsetVertices(0);
	uint32_t			offsetIndices(0);

	const aiScene* pScene = importer.ReadFile(filename, pFlags);

	if (!pScene || !pScene->mRootNode || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		std::cout << "[Error] : " << importer.GetErrorString() << std::endl;
		throw std::runtime_error("Error at ModelLoader::loadModel!");
	}
	else {
		std::cout << "[DEBUG] \tSuccess at loading object from: " << filename << std::endl;
		std::cout << "[DEBUG] \tModel Attributes: \t" << "HasTextures | HasMaterials\t" << pScene->HasTextures() << " | " << pScene->HasMaterials() << std::endl;
		
		m_modelPaths.push_back(filename);
	}

	bool isFbx(filename.find(".fbx"));

	/* Clear temp storage for next PJModel */
	if (!m_meshesOfCurrentModel.empty())
		m_meshesOfCurrentModel.clear();

	/* Loads meshes recursively to maintain relation tree */
	recurseNodes(pScene->mRootNode, pScene, centerModel, isFbx, offsetVertices, offsetIndices);

	PJModel temp_PJModel{};
	temp_PJModel.meshes.insert(temp_PJModel.meshes.end(), m_meshesOfCurrentModel.begin(), m_meshesOfCurrentModel.end());
	temp_PJModel.modelPath = filename;
	temp_PJModel.centered = centerModel;

	/* Loads embedded textures from fbx File */
	if (isFbx != std::string::npos) {
		std::cout << "[DEBUG] \tLoading embedded textures from: " << filename << std::endl;
		temp_PJModel.textures = loadTextureFromFBX(pScene);
	}

	/* Preparation for next call of loadModel */
	++m_activeModels;

	return temp_PJModel;
}

void pje::ModelLoader::recurseNodes(aiNode* node, const aiScene* pScene, bool centerModel, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices) {
	// extract all mesh data from this node
	for (uint32_t i = 0; i < node->mNumMeshes; i++) {
		// aiNode only knows index of aiMesh in aiScene
		aiMesh* mesh = pScene->mMeshes[node->mMeshes[i]];
		m_meshesOfCurrentModel.push_back(convertMesh(mesh, pScene, centerModel, isFbx, offsetVertices, offsetIndices));
	}

	// go through each child's node (and all its children) in sequence
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		// aiNode's children only know index of aiMesh in aiScene
		recurseNodes(node->mChildren[i], pScene, centerModel, isFbx, offsetVertices, offsetIndices);
	}
}

pje::PJMesh pje::ModelLoader::convertMesh(aiMesh* mesh, const aiScene* pScene, bool centerModel, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices) {
	std::vector<PJVertex> vertices;
	std::vector<uint32_t> indices;

	// copy VBO
	if (mesh->HasNormals()) {
		for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
			if (isFbx) {
				PJVertex currentVertex(
					{ mesh->mVertices[i].x, mesh->mVertices[i].z, mesh->mVertices[i].y },	// glm::vec3 m_position
					{ 1.0f, 1.0f, 0.0f },													// glm::vec3 m_color	
					{ mesh->mNormals[i].x, mesh->mNormals[i].z, mesh->mNormals[i].y }		// glm::vec3 m_normal
				);

				vertices.push_back(currentVertex);
			}
			else {
				PJVertex currentVertex(
					{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },	// glm::vec3 m_position
					{ 1.0f, 1.0f, 0.0f },													// glm::vec3 m_color	
					{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }		// glm::vec3 m_normal
				);

				vertices.push_back(currentVertex);
			}
		}
	}
	else {
		throw std::runtime_error("Error at ModelLoader::translateMesh : Mesh has no normals!");
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

	offsetVertices += vertices.size();
	offsetIndices += indices.size();

	/* returns PJMesh without own offset expansion */
	return PJMesh(vertices, indices, offsetVertices - vertices.size(), offsetIndices - indices.size());
}

std::vector<const aiTexture*> pje::ModelLoader::loadTextureFromFBX(const aiScene* pScene) {
	std::vector<const aiTexture*> modelTextures;

	std::cout << "[DEBUG] \tEmbedded Textures : " << pScene->mNumTextures << " | " << " Embedded Materials : " << pScene->mNumMaterials << std::endl;

	if (pScene->HasMaterials()) {
		aiReturn result;
		aiString materialName;
		aiString currentTexturePath;
		aiMaterial* currentMaterial;

		for (uint32_t i = 0; i < pScene->mNumMaterials; i++) {
			currentMaterial = pScene->mMaterials[i];
			
			if (currentMaterial->Get(AI_MATKEY_NAME, materialName) == AI_FAILURE)
				materialName = "ERROR_NO_MATERIAL_NAME";

			std::cout << "[DEBUG] \tName of current aiMaterial:\t" << materialName.C_Str() << std::endl;

			if (currentMaterial->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE) > 0) {
				if (currentMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType::aiTextureType_DIFFUSE, 0), currentTexturePath) != AI_FAILURE) {
					const aiTexture* texture = pScene->GetEmbeddedTexture(currentTexturePath.C_Str());
					std::cout << "[DEBUG] \tCurrent aiTexture\t Width : " << texture->mWidth << " | " << " Height : " << texture->mHeight << std::endl;
					modelTextures.push_back(texture);
				}
			}
			else {
				std::cout << "[DEBUG] \tCurrent aiMaterial has no albedo/diffuse texture" << std::endl;
			}
		}
	}

	return modelTextures;
}