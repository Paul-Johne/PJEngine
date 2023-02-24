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

	private:
		std::filesystem::path		m_folderForModels;
		std::vector<PJMesh>			m_temp_meshesOfModel;
		uint32_t					m_offsetsCurrentPJModel;

		void recurseNodes(aiNode* node, const aiScene* pScene, bool centerModel);
		PJMesh convertMesh(aiMesh* mesh, const aiScene* pScene, bool centerModel);

		std::vector<const aiTexture*> loadTextureFromFBX(const aiScene* pScene);
	};
}