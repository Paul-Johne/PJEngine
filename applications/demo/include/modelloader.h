/* Ignores header if not needed after #include */
	#pragma once

	#include <filesystem>
	#include <iostream>
	#include <memory>
	#include <vector>

	#include <assimp/Importer.hpp>
	#include <assimp/scene.h>
	#include <assimp/postprocess.h>

	#include <globalParams.h>

namespace pje {
	
	/* #### ModelLoader - Holds all loaded objects with their respective path for PJEngine #### */
	class ModelLoader {
	public:
		std::vector<PJMesh>	m_models;
		std::vector<std::string> m_modelPaths;
		int	m_active_models;

		ModelLoader() : m_models(), m_modelPaths(), m_active_models(0), folderForModels("assets/models") {
			for (const auto& each : std::filesystem::directory_iterator(folderForModels)) {
				loadModel(each.path().string());
			}
		}

		~ModelLoader() 
			{}

		void loadModel(const std::string& filename) {
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

			recurseNodes(pScene->mRootNode, pScene);
		}

	private:
		std::filesystem::path folderForModels;

		void recurseNodes(aiNode* node, const aiScene* pScene) {
			// extract all mesh data from this node
			for (uint32_t i = 0; i < node->mNumMeshes; i++) {
				aiMesh* mesh = pScene->mMeshes[node->mMeshes[i]];
				m_models.push_back(translateMesh(mesh, pScene));
			}

			// go through each child's node (and all its children) in sequence
			for (uint32_t i = 0; i < node->mNumChildren; i++) {
				recurseNodes(node->mChildren[i], pScene);
			}
		}

		PJMesh translateMesh(aiMesh* mesh, const aiScene* pScene) {
			std::vector<PJVertex> vertices;
			std::vector<uint32_t> indices;

			// copy VBO
			for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
				PJVertex currentVertex(
					{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
					{ 0.0f, 0.0f, 0.0f }
				);

				vertices.push_back(currentVertex);
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
	};
}