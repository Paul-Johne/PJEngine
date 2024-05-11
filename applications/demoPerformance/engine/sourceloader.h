/* Ignores header if not needed after #include */
	#pragma once

	#include <cstdint>				// fixed size integer
	#include <string>				// std::string

	#include <iostream>				// i/o stream
	#include <filesystem>			// file paths

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
		std::vector<pje::engine::types::Primitive>	m_primitives;
		std::vector<std::string>					m_primitivePaths;
		uint8_t										m_activePrimitivesCount;
		std::vector<std::vector<unsigned char>>		m_textures;

		Sourceloader();
		~Sourceloader();
	private:
		bool					m_centerPrimitive;
		std::filesystem::path	m_sourceFolder;

		/* loads primitives and their textures into m_primitives and m_textures */
		void loadPrimitive(const std::string& filename, unsigned int flags, bool centerPrimitive);
		/* Assimp (row major) -> glm (column major) */
		static glm::mat4 matrix4x4Assimp2glm(const aiMatrix4x4& matrix);
	};
}