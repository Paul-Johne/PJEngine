#include <globalParams.h>

void pje::PJModel::prepareTexture(const aiTexture* compressedRaw, size_t dstIndex, uint8_t desiredChannels, bool newEntry) {
	int len;

	if (compressedRaw->mHeight == 0 && compressedRaw->mWidth > 0)
		len = compressedRaw->mWidth;
	else
		len = compressedRaw->mWidth * compressedRaw->mHeight;

	unsigned char* pixels = stbi_load_from_memory(
		reinterpret_cast<unsigned char*>(compressedRaw->pcData),
		len,
		&this->m_textureInfos[dstIndex].x_width,								// saves data inside
		&this->m_textureInfos[dstIndex].y_height,								// saves data inside
		&this->m_textureInfos[dstIndex].channels,								// saves data inside
		desiredChannels
	);

	this->m_textureInfos[dstIndex].size = this->m_textureInfos[dstIndex].x_width * this->m_textureInfos[dstIndex].y_height * desiredChannels;
	std::vector<unsigned char> res(pixels, pixels + m_textureInfos[dstIndex].size);
	stbi_image_free(pixels);

	if (newEntry)
		this->m_uncompressedTextures.push_back(res);
	else
		this->m_uncompressedTextures[dstIndex] = res;

	return;
}

// ##################################################################################################################################################################

pje::Context pje::context = {};

pje::Uniforms pje::uniforms = {
	glm::mat4(1.0f),
	glm::mat4(1.0f),
	glm::mat4(1.0f),
	glm::mat4(1.0f)
};

std::vector<pje::PJVertex> pje::debugVertices = {
	pje::PJVertex({ -0.5f, 0.0f, -0.5f}, {5.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({  0.5f, 0.0f, -0.5f}, {0.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({ -0.5f, 0.0f,  0.5f}, {0.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}),
	pje::PJVertex({  0.5f, 0.0f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f})
};
std::vector<uint32_t> pje::debugIndices = {
	0, 2, 1,
	1, 2, 3
};

std::vector<pje::PJMesh> pje::debugMesh = {
	pje::PJMesh(pje::debugVertices, pje::debugIndices, 0, 0)
};

std::vector<pje::PJModel> pje::loadedModels = {};

pje::PJBuffer pje::stagingBuffer = {};

/* One PJModel with all its PJMeshes */
pje::PJBuffer pje::vertexBuffer = {};
pje::PJBuffer pje::indexBuffer = {};

/* Holds matrices for shader computation */
pje::PJBuffer pje::uniformsBuffer = {};

std::vector<pje::BoneRef> boneRefs = {};
pje::PJBuffer pje::storeBoneRefs = {};
pje::PJBuffer pje::storeBoneMatrices = {};

/* Vulkan "Texture Units" */
pje::PJImage pje::texAlbedo = {};
pje::PJImage pje::rtMsaa = {};
pje::PJImage pje::rtDepth = {};