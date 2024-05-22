#include "sourceloader.h"

#define ANTI_FBX_SCALE 0.01f
#define DEFAULT_ASSIMP_FLAGS aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices

pje::engine::Sourceloader::Sourceloader() : m_primitives(), 
											m_primitivePaths(), 
											m_activePrimitivesCount(0),  
											m_centerPrimitive(false), 
											m_sourceFolder("assets/primitives") {
	/* looking for .fbx elements in m_sourceFolder to load data from those files */
	for (const auto& each : std::filesystem::directory_iterator(m_sourceFolder)) {
		auto path = each.path().string();

		if (path.find(".fbx") != std::string::npos) {
			std::cout << "[PJE] \t.fbx file found at: " << path << std::endl;
			loadPrimitive(path, DEFAULT_ASSIMP_FLAGS, m_centerPrimitive);
			std::cout << "[PJE] \Importing fbx => primitive --- DONE" << std::endl;
		}
	}
}

pje::engine::Sourceloader::~Sourceloader() {}

void pje::engine::Sourceloader::loadPrimitive(const std::string& filename, unsigned int flags, bool centerPrimitive) {
	std::vector<pje::engine::types::Mesh>	currentMeshes;		// empty mesh collector for primitive
	uint32_t								offsetVertices(0);	// helper to set Primitive::m_meshes' offsets
	uint32_t								offsetIndices(0);	// helper to set Primitive::m_meshes' offsets

	Assimp::Importer	importer;
	const aiScene*		pScene = importer.ReadFile(filename, flags);

	if (!pScene || !pScene->mRootNode || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		throw std::runtime_error("Assimp cannot load primitive's data!");
	}
	else {
		/* loads existing data via assimp into currentMeshes to insert it into currentPrimitive */
		recurseAiNode2LoadMeshes(pScene->mRootNode, pScene, currentMeshes, offsetVertices, offsetIndices);

		/* instantiating temporary primitive to stack onto m_primitives later */
		pje::engine::types::Primitive currentPrimitive{};

		m_primitivePaths.push_back(filename);
		currentPrimitive.m_meshes.insert(currentPrimitive.m_meshes.end(), currentMeshes.begin(), currentMeshes.end());

		/* additional centering of vertices in its local space (only necessary when 3D model wasn't exported correctly) */
		if (centerPrimitive) {
			std::cout << "[PJE] \tCentering primitive's vertices.." << std::endl;
			glm::vec3	avgPos{ 0.0f, 0.0f, 0.0f };
			size_t		vCount = 0;

			for (const auto& mesh : currentPrimitive.m_meshes) {
				vCount += mesh.m_vertices.size();
				for (const auto& v : mesh.m_vertices) {
					avgPos += v.m_pos;
				}
			}

			avgPos /= vCount;

			for (auto& mesh : currentPrimitive.m_meshes) {
				std::for_each(
					std::execution::par_unseq,
					mesh.m_vertices.begin(),
					mesh.m_vertices.end(),
					[&](pje::engine::types::Vertex& v) {
						v.m_pos -= avgPos;
					}
				);
			}
			std::cout << "[PJE] \tCentering done." << std::endl;
		}

		/* loads certain texture into predestined texture slot (m_texture) of currentPrimitive */
		loadTextureTypeFor(currentPrimitive, "albedo", STBI_rgb_alpha, pScene);

		m_primitives.push_back(currentPrimitive);
		++m_activePrimitivesCount;
	}

	return;
}

void pje::engine::Sourceloader::loadTextureTypeFor(pje::engine::types::Primitive& primitive, const std::string& type, uint8_t texChannels, const aiScene* pScene) {
	if (pScene->HasTextures()) {
		aiTexture*	rawTexture;
		std::string	currentFilename;

		/* goes through all available textures of pScene to get the first texture of the required type */
		for (unsigned int i = 0; i < pScene->mNumTextures; i++) {
			rawTexture = pScene->mTextures[i];
			currentFilename = rawTexture->mFilename.C_Str();

			if (currentFilename.find(type) != std::string::npos) {
				size_t pixelCount;

				if (rawTexture->mHeight == 0 && rawTexture->mWidth > 0)
					pixelCount = rawTexture->mWidth;
				else
					pixelCount = rawTexture->mWidth * rawTexture->mHeight;

				unsigned char* pixels = stbi_load_from_memory(
					reinterpret_cast<unsigned char*>(rawTexture->pcData),
					pixelCount,
					&primitive.m_texture.width,		// set by stb
					&primitive.m_texture.height,	// set by stb
					&primitive.m_texture.channels,	// set by stb
					texChannels
				);

				primitive.m_texture.name = currentFilename;
				primitive.m_texture.size = primitive.m_texture.width * primitive.m_texture.height * texChannels;
				std::vector<unsigned char> uncompressedTexture(pixels, pixels + primitive.m_texture.size);
				primitive.m_texture.uncompressedTexture = uncompressedTexture;

				stbi_image_free(pixels);
				return;
			}
			/* if no texture of the required type was found an error will be thrown */
			else {
				throw std::runtime_error("Current primitive's texture has no name for type evaluation!");
			}
		}
	}
	else {
		throw std::runtime_error("Current primitive does not provide a texture!");
	}
}

void pje::engine::Sourceloader::recurseAiNode2LoadMeshes(aiNode* pNode,
														 const aiScene* pScene, 
														 std::vector<pje::engine::types::Mesh>& meshes, 
														 uint32_t& offsetVertices, 
														 uint32_t& offsetIndices, 
														 glm::mat4 nodeTransform) {
	/* M x T (local translation to recursively update currentTransform => both matrices are in the same space) */
	glm::mat4 currentTransform(nodeTransform * matrix4x4Assimp2glm(pNode->mTransformation));

	/* extracting meshes of current pNode */
	for (unsigned int i = 0; i < pNode->mNumMeshes; i++) {
		aiMesh* pMesh = pScene->mMeshes[pNode->mMeshes[i]];
		meshes.push_back(meshAssimp2PJE(pMesh, pScene, offsetVertices, offsetIndices, currentTransform));
	}

	/* extracting meshes of each child node in sequence */
	for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
		recurseAiNode2LoadMeshes(pNode->mChildren[i], pScene, meshes, offsetVertices, offsetIndices, currentTransform);
	}
}

pje::engine::types::Mesh pje::engine::Sourceloader::meshAssimp2PJE(aiMesh* pMesh, 
																   const aiScene* pScene, 
																   uint32_t& offsetVertices, 
																   uint32_t& offsetIndices, 
																   const glm::mat4& nodeTransform) {
	/* this function extracts data of meshes to populate and return pje::engine::types::Mesh */
	std::vector<pje::engine::types::Vertex>	vertices;
	std::vector<uint32_t>					indices;

	if (pMesh->HasPositions() && pMesh->HasNormals() && pMesh->HasTextureCoords(0) && pMesh->HasFaces()) {
		/* copying VBO */
		for (unsigned int i = 0; i < pMesh->mNumVertices; i++) {
			pje::engine::types::Vertex v(
				{ pMesh->mVertices[i].x,			pMesh->mVertices[i].y,			pMesh->mVertices[i].z },	// glm::vec3 pos
				{ pMesh->mNormals[i].x,				pMesh->mNormals[i].y,			pMesh->mNormals[i].z },		// glm::vec3 normal
				{ pMesh->mTextureCoords[0][i].x,	pMesh->mTextureCoords[0][i].y}								// glm::vec2 uv of texture set 0
			);
			vertices.push_back(v);
		}
	}
	else {
		throw std::runtime_error("Assimp mesh does not comply with engine's mesh!");
	}

	/* applying initial vertex transforms in parallel */
	std::for_each(
		std::execution::par_unseq,
		vertices.begin(),
		vertices.end(),
		[&](pje::engine::types::Vertex& v) {
			v.m_pos		= glm::vec3(nodeTransform * glm::vec4(v.m_pos, 1.0f)) * ANTI_FBX_SCALE;
			v.m_normal	= glm::normalize(glm::transpose(glm::inverse(nodeTransform)) * glm::vec4(v.m_normal, 0.0f));
		}
	);

	/* copying IBO */
	for (unsigned int i = 0; i < pMesh->mNumFaces; i++) {
		aiFace face = pMesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	/* side effect variables for possible further meshes in current primitive */
	offsetVertices	+= vertices.size();
	offsetIndices	+= indices.size();

	return pje::engine::types::Mesh(vertices, indices, offsetVertices - vertices.size(), offsetIndices - indices.size());
}

glm::mat4 pje::engine::Sourceloader::matrix4x4Assimp2glm(const aiMatrix4x4& assimpMatrix) {
	glm::mat4 glmMatrix;

	/* populates glm matrix with assimp matrix data */
	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			glmMatrix[column][row] = assimpMatrix[row][column];
		}
	}

	return glmMatrix;
}