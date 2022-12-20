/* Ignores header if not needed after #include */
	#pragma once

	#include <iostream>
	#include <memory>
	#include <vector>

	#include <assimp/Importer.hpp>
	#include <assimp/scene.h>
	#include <assimp/postprocess.h>

	#include <globalParams.h>

namespace pje {
	
	/* #### ModelLoader #### */
	class ModelLoader {
	public:
		ModelLoader() {};
		~ModelLoader() {};

		void loadModel(const std::string& filename) {
			Assimp::Importer importer;

			const aiScene* pScene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

			if (!pScene) {
				std::cout << "Error at ModelLoader::loadModel : " << importer.GetErrorString() << std::endl;
				throw std::runtime_error("Error at ModelLoader::loadeModel!");
			}
			else {
				std::cout << "Success at ModelLoader::loadModel!" << std::endl;
			}
		}

	private:
		std::vector<int>	m_models;
		int					m_activeModels;
	};
	extern std::unique_ptr<pje::ModelLoader> modelLoader;
}