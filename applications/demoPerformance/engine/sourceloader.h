/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>				// fixed size integer
	#include <string>				// std::string
	#include <vector>				// std::vector

	#include <iostream>				// i/o stream
	#include <filesystem>			// file paths
	#include <algorithm>			// classic functions for ranges
	#include <execution>			// parallel algorithms

	#include <assimp/scene.h>		// Assimp: data structure
	#include <assimp/Importer.hpp>	// Assimp: importer interface
	#include <assimp/postprocess.h>	// Assimp: post processing flags
	#include <stb_image.h>			// stb
	#include <glm/glm.hpp>			// glm

	#include "pjeBuffers.h"

namespace pje::engine {

	/* Sourceloader - Loads FBX primitives with embedded textures */
	class Sourceloader {
	public:
		std::vector<pje::engine::types::Primitive>	m_primitives;				// data
		std::vector<std::string>					m_primitivePaths;			// m_primitivePaths[i] => m_primitives[i]
		uint8_t										m_activePrimitivesCount;	// size(m_primitives)

		Sourceloader();
		~Sourceloader();
	private:
		bool					m_centerPrimitive;
		std::filesystem::path	m_sourceFolder;

		/* loads primitives and their textures into m_primitives and m_textures */
		void loadPrimitive(const std::string& filename, unsigned int flags, bool centerPrimitive);

		/* loads decompressed texture of a certain type for the given primitive */
		void loadTextureTypeFor(pje::engine::types::Primitive& primitive, const std::string& type, uint8_t texChannels, const aiScene* pScene);

		/* recurses aiNode(s) to collect all mesh data of current primitive */
		void recurseAiNode2LoadMeshes(
			aiNode* pNode, 
			const aiScene* pScene, 
			std::vector<pje::engine::types::Mesh>& meshes, 
			uint32_t& offsetVertices, 
			uint32_t& offsetIndices, 
			glm::mat4 nodeTransform = glm::mat4(1.0f)
		);

		/* converts aiMesh data into engines mesh type */
		pje::engine::types::Mesh meshAssimp2PJE(
			aiMesh* pMesh, 
			const aiScene* pScene, 
			uint32_t& offsetVertices, 
			uint32_t& offsetIndices, 
			const glm::mat4& nodeTransform
		);

		/* Assimp (row major) -> glm (column major) */
		static glm::mat4 matrix4x4Assimp2glm(const aiMatrix4x4& matrix);
	};
}