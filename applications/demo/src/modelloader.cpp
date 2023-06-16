#include <modelloader.h>

#include <iostream>
#include <memory>
#include <algorithm>
#include <execution>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#define DEFAULT_ASSIMP_FLAGS aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs

pje::Modelloader::Modelloader() : m_models(), m_modelPaths(), m_activeModels(0), m_centerModel(true), m_folderForModels("assets/models") {
	for (const auto& each : std::filesystem::directory_iterator(m_folderForModels)) {
		auto path = each.path().string();

		if (path.find(".fbx") != std::string::npos || path.find(".obj") != std::string::npos) {
			std::cout << "[DEBUG] 3D object found at: " << path << std::endl;
			m_models.push_back(loadModel(path, DEFAULT_ASSIMP_FLAGS, m_centerModel));
		}
	}
}

pje::Modelloader::~Modelloader() {}

pje::PJModel pje::Modelloader::loadModel(const std::string& filename, unsigned int pFlags, bool centerModel) {
	Assimp::Importer	importer;
	uint32_t			offsetVertices(0);
	uint32_t			offsetIndices(0);

	const aiScene* pScene = importer.ReadFile(filename, pFlags);

	if (!pScene || !pScene->mRootNode || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		std::cout << "[Error] : " << importer.GetErrorString() << std::endl;
		throw std::runtime_error("Error at Modelloader::loadModel!");
	}
	else {
		std::cout << "[DEBUG] \tSuccess at loading object from: " << filename << std::endl;
		std::cout << "[DEBUG] \tModel Attributes: \t" << "HasTextures | HasMaterials\t" << pScene->HasTextures() << " | " << pScene->HasMaterials() << std::endl;
		
		m_modelPaths.push_back(filename);
	}

	bool isFbx(filename.find(".fbx") != std::string::npos);

	/* Clear temp storage for next PJModel */
	if (!m_meshesOfCurrentModel.empty())
		m_meshesOfCurrentModel.clear();

	/* Loads meshes recursively to maintain relation tree */
	recurseNodes(pScene->mRootNode, pScene, isFbx, offsetVertices, offsetIndices);

	PJModel temp_PJModel{};
	temp_PJModel.m_meshes.insert(temp_PJModel.m_meshes.end(), m_meshesOfCurrentModel.begin(), m_meshesOfCurrentModel.end());
	temp_PJModel.m_modelPath = filename;
	temp_PJModel.m_centered = centerModel;

	if (centerModel) {
		glm::vec3 avgPos{ 0.0f, 0.0f, 0.0f };
		size_t vertexCount = 0;

		for (const auto& mesh : temp_PJModel.m_meshes) {
			vertexCount += mesh.m_vertices.size();

			for (const auto& v : mesh.m_vertices) {
				avgPos += v.m_position;
			}
		}

		avgPos /= vertexCount;

		for (auto& mesh : temp_PJModel.m_meshes) {
			std::for_each(
				std::execution::par_unseq,
				mesh.m_vertices.begin(),
				mesh.m_vertices.end(),
				[&](pje::PJVertex& v) {
					v.m_position -= avgPos;
				}
			);
		}
	}
	
	/* Loads embedded textures from fbx File */
	if (isFbx) {
		std::cout << "[DEBUG] \tLoading embedded textures from: " << filename << std::endl;
		loadTextureFromFBX(temp_PJModel, pScene);
	}

	/* Preparation for next call of loadModel */
	++m_activeModels;

	return temp_PJModel;
}

glm::mat4 pje::Modelloader::convertToColumnMajor(const aiMatrix4x4& matrix) {
	glm::mat4 res;
	
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			// MATHE : res[c * 4 + r] = matrix[r * 4 + r]
			res[c][r] = matrix[r][c];
		}
	}

	return res;
}

void pje::Modelloader::recurseNodes(aiNode* node, const aiScene* pScene, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices, glm::mat4 accTransform) {
	glm::mat4 transform(accTransform * convertToColumnMajor(node->mTransformation));
	
	// extract all mesh data from this node
	for (uint32_t i = 0; i < node->mNumMeshes; i++) {
		// aiNode only knows index of aiMesh in aiScene
		aiMesh* mesh = pScene->mMeshes[node->mMeshes[i]];
		m_meshesOfCurrentModel.push_back(convertMesh(mesh, pScene, isFbx, offsetVertices, offsetIndices, transform));
	}

	// go through each child's node (and all its children) in sequence
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		// aiNode's children only know index of aiMesh in aiScene
		recurseNodes(node->mChildren[i], pScene, isFbx, offsetVertices, offsetIndices, transform);
	}
}

pje::PJMesh pje::Modelloader::convertMesh(aiMesh* mesh, const aiScene* pScene, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices, const glm::mat4& accTransform) {
	std::vector<PJVertex> vertices;
	std::vector<uint32_t> indices;

	// Copy VBO
	if (mesh->HasNormals()) {
		for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
			PJVertex currentVertex(
				{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },	// glm::vec3 m_position
				{ 1.0f, 1.0f, 0.0f },													// glm::vec3 m_color	
				{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z },		// glm::vec3 m_normal
				{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}			// TODO(tertiary/ternary expression!!!) 
			);

			vertices.push_back(currentVertex);
		}
	}
	else {
		throw std::runtime_error("Error at Modelloader::translateMesh : Mesh has no normals!");
	}

	// Apply initial tranform to vertices in parallel | [&] capture clause => makes accTransform visible
	std::for_each(
		std::execution::par_unseq, 
		vertices.begin(), 
		vertices.end(), 
		[&](pje::PJVertex& v) {
			v.m_position = glm::vec3(accTransform * glm::vec4(v.m_position, 1.0f));
			v.m_normal = glm::normalize(glm::vec3(glm::transpose(glm::inverse(accTransform)) * glm::vec4(v.m_normal, 0.0f)));
		}
	);

	// Rescale each vertex in parallel
	if (isFbx) {
		std::for_each(
			std::execution::par_unseq,
			vertices.begin(),
			vertices.end(),
			[&](pje::PJVertex& v) {
				v.m_position *= pje::config::antiFbxScale;
			}
		);
	}

	// Copy IBO
	for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	offsetVertices += vertices.size();
	offsetIndices += indices.size();

	/* Returns PJMesh without own offset expansion */
	return PJMesh(vertices, indices, offsetVertices - vertices.size(), offsetIndices - indices.size());
}

void pje::Modelloader::loadTextureFromFBX(pje::PJModel& fbx, const aiScene* pScene) {
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

					fbx.m_textureInfos.push_back(pje::TextureInfo());
					fbx.prepareTexture(texture, i);
				}
			}
			else {
				std::cout << "[DEBUG] \tCurrent aiMaterial has no albedo/diffuse texture" << std::endl;
			}
		}
	}

	return;
}