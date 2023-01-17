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
		std::vector<PJMesh>	m_models;
		std::vector<std::string> m_modelPaths;
		int	m_active_models;
		bool m_centerModel;

		ModelLoader();
		~ModelLoader();
		void loadModel(const std::string& filename, bool centerModel = false);

	private:
		std::filesystem::path m_folderForModels;

		void recurseNodes(aiNode* node, const aiScene* pScene, bool centerModel);
		PJMesh translateMesh(aiMesh* mesh, const aiScene* pScene, bool centerModel);
	};
}