/* Ignores header if not needed after #include */
	#pragma once

	#include <filesystem>
	#include <vector>

	#include <assimp/scene.h>

	#include <globalParams.h>

namespace pje {

	/* #### ModelLoader - Holds all loaded objects with their respective path for PJEngine #### */
	class ModelLoader {
	public:
		std::vector<PJModel>		m_models;
		std::vector<std::string>	m_modelPaths;
		int							m_activeModels;
		bool						m_centerModel;

		ModelLoader();
		~ModelLoader();

		pje::PJModel loadModel(const std::string& filename, unsigned int pFlags, bool centerModel = false);
		static glm::mat4 convertToColumnMajor(const aiMatrix4x4& matrix);

	private:
		std::filesystem::path		m_folderForModels;
		std::vector<PJMesh>			m_meshesOfCurrentModel;

		void recurseNodes(aiNode* node, const aiScene* pScene, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices, glm::mat4 accTransform = glm::mat4(1.0f));
		PJMesh convertMesh(aiMesh* mesh, const aiScene* pScene, bool isFbx, uint32_t& offsetVertices, uint32_t& offsetIndices, const glm::mat4& accTransform);

		void pje::ModelLoader::loadTextureFromFBX(pje::PJModel& fbx, const aiScene* pScene);
	};
}