#include "rendererGL.h"
//#define DEBUG

/* ### Public methods ### */

pje::renderer::RendererGL::RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable) :
	m_handles(), m_renderWidth(parser.m_width), m_renderHeight(parser.m_height), m_windowIconified(false), 
	m_vsync(parser.m_vsync), m_anisotropyLevel(AnisotropyLevel::TWOx), m_msaaFactor(4), m_instanceCount(parser.m_amountOfObjects) {

	/* Gaining access to OpenGL core functions inside of C++ */
	glfwMakeContextCurrent(window);
	if (gl3wInit()) {
		glfwTerminate();
		throw std::runtime_error("Init of gl3w failed.");
	}
	std::cout << "[GL3W] \tOpenGL Version: " << glGetString(GL_VERSION) << "\n\t[GPU]\t" << glGetString(GL_RENDERER) << std::endl;

	/* VSync Option */
	if (!m_vsync)
		glfwSwapInterval(0);
	else
		glfwSwapInterval(1);

	/* Defining general info for renderloop */
	setGlobalSettings();
	/* Creating shader program */
	setShaderProgram("basic_opengl", m_handles.shaderProgram);
	/* Using shader program */
	glUseProgram(m_handles.shaderProgram);
}

pje::renderer::RendererGL::~RendererGL() {
	// Cleanup of OpenGL ressources (optional)
	glDeleteBuffers(1, &m_buffUniformMVP);
	glDeleteBuffers(1, &m_buffStorageBoneRefs);
	glDeleteBuffers(1, &m_buffStorageBones);
	glDeleteProgram(m_handles.shaderProgram);
}

void pje::renderer::RendererGL::uploadRenderable(const pje::engine::types::LSysObject& renderable) {
	glGenVertexArrays(1, &m_handles.buffRenderable.vaoHandle);
	glGenBuffers(1, &m_handles.buffRenderable.verticesHandle);
	glGenBuffers(1, &m_handles.buffRenderable.indicesHandle);

	/* Binding VAO for VBO and IBO/EBO */
	glBindVertexArray(m_handles.buffRenderable.vaoHandle);

#ifdef DEBUG
	std::cout << "[GL3W] \tsizeof(Vertex): " << sizeof(pje::engine::types::Vertex) << std::endl;
#endif // DEBUG

	/* 1) Vertices */
	m_handles.buffRenderable.verticesSize = static_cast<signed long long>(
		(
			renderable.m_objectPrimitives.back().m_meshes.back().m_vertices.size() +
			renderable.m_objectPrimitives.back().m_meshes.back().m_offsetPriorMeshesVertices +
			renderable.m_objectPrimitives.back().m_offsetPriorPrimitivesVertices
		) * sizeof(pje::engine::types::Vertex)
	);
	glBindBuffer(GL_ARRAY_BUFFER, m_handles.buffRenderable.verticesHandle);
	glBufferData(GL_ARRAY_BUFFER, m_handles.buffRenderable.verticesSize, NULL, GL_STATIC_DRAW);

	/* 2) Indices */
	m_handles.buffRenderable.indicesSize = static_cast<signed long long>(
		(
			renderable.m_objectPrimitives.back().m_meshes.back().m_indices.size() +
			renderable.m_objectPrimitives.back().m_meshes.back().m_offsetPriorMeshesIndices +
			renderable.m_objectPrimitives.back().m_offsetPriorPrimitivesIndices
		) * sizeof(uint32_t)
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handles.buffRenderable.indicesHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_handles.buffRenderable.indicesSize, NULL, GL_STATIC_DRAW);

	/* 3) Uploading */
	size_t vByteOffset = 0;
	size_t vByteSize;
	size_t iByteOffset = 0;
	size_t iByteSize;

	for (const auto& primitive : renderable.m_objectPrimitives) {
		for (const auto& mesh : primitive.m_meshes) {
			vByteSize = sizeof(pje::engine::types::Vertex) * mesh.m_vertices.size();
			iByteSize = sizeof(uint32_t) * mesh.m_indices.size();

			glBufferSubData(GL_ARRAY_BUFFER, vByteOffset, vByteSize, mesh.m_vertices.data());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iByteOffset, iByteSize, mesh.m_indices.data());

			vByteOffset += vByteSize;
			iByteOffset += iByteSize;
		}
	}

	/* 4) Vertex Attributes of pje::::engine::types::Vertex */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT, false, sizeof(pje::engine::types::Vertex), (void*)offsetof(pje::engine::types::Vertex, m_pos)
	);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, false, sizeof(pje::engine::types::Vertex), (void*)offsetof(pje::engine::types::Vertex, m_normal)
	);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 2, GL_FLOAT, false, sizeof(pje::engine::types::Vertex), (void*)offsetof(pje::engine::types::Vertex, m_uv)
	);
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(
		3, 2, GL_UNSIGNED_INT, sizeof(pje::engine::types::Vertex), (void*)offsetof(pje::engine::types::Vertex, m_boneAttrib)
	);

	/* 5) Unbinding */
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	std::cout << "[GL3W] \tUploading renderable --- DONE" << std::endl;
}

void pje::renderer::RendererGL::uploadTextureOf(const pje::engine::types::LSysObject& renderable, bool genMipmaps, TextureType type) {
	switch (type) {
	case TextureType::Albedo:
		glGenTextures(1, &m_texAlbedo.handle);
		glBindTexture(GL_TEXTURE_2D, m_texAlbedo.handle);

		/* Uploading texture with mipmap option */
		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_RGBA8, 
			renderable.m_choosenTexture.width, 
			renderable.m_choosenTexture.height, 
			0, 
			GL_RGBA, 
			GL_UNSIGNED_BYTE, 
			renderable.m_choosenTexture.uncompressedTexture.data()
		);
		if (genMipmaps)
			glGenerateMipmap(GL_TEXTURE_2D);

		/* Setting texture parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (genMipmaps) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		float anisotropyVal = 1.0f;
		switch (m_anisotropyLevel) {
		case AnisotropyLevel::Disabled:
			anisotropyVal = 1.0f;
			break;
		case AnisotropyLevel::TWOx:
			anisotropyVal = 2.0f;
			break;
		case AnisotropyLevel::FOURx:
			anisotropyVal = 4.0f;
			break;
		case AnisotropyLevel::EIGHTx:
			anisotropyVal = 8.0f;
			break;
		case AnisotropyLevel::SIXTEENx:
			anisotropyVal = 16.0f;
			break;
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, anisotropyVal);

		/* Setting proper ImageGL when binding name for shader */
		m_texAlbedo.samplerName = "albedo";

		glBindTexture(GL_TEXTURE_2D, 0);
		break;
	}
	std::cout << "[GL3W] \tUploading texture of a renderable --- DONE" << std::endl;
}

void pje::renderer::RendererGL::uploadBuffer(const pje::engine::types::LSysObject& renderable, BufferType type) {
	switch (type) {
	case BufferType::UniformMVP:
		/* explicit uniform block */
		glGenBuffers(1, &m_buffUniformMVP);
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffUniformMVP);
		glBufferData(
			GL_UNIFORM_BUFFER, 
			sizeof(pje::engine::types::MVPMatrices), 
			&renderable.m_matrices, 
			GL_STATIC_DRAW
		);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		break;
	case BufferType::StorageBoneRefs:
		/* explicit storage buffer location/index = 0 */
		glGenBuffers(1, &m_buffStorageBoneRefs);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffStorageBoneRefs);
		glBufferData(
			GL_SHADER_STORAGE_BUFFER, 
			sizeof(pje::engine::types::BoneRef) * renderable.m_boneRefs.size(), 
			renderable.m_boneRefs.data(), 
			GL_STATIC_DRAW
		);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		break;
	case BufferType::StorageBones:
		/* explicit storage buffer location/index = 1 */
		glGenBuffers(1, &m_buffStorageBones);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffStorageBones);
		glBufferData(
			GL_SHADER_STORAGE_BUFFER,
			sizeof(glm::mat4) * renderable.m_bones.size(),
			renderable.getBoneMatrices().data(),
			GL_STATIC_DRAW
		);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		break;
	}
}

void pje::renderer::RendererGL::bindRenderable(const pje::engine::types::LSysObject& renderable) {
	/* Every resource that needs to be linked before glDraw*() */

	/* Binding m_texAlbedo to Texture Unit 0 */
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, m_texAlbedo.handle);
	glUniform1i(
		glGetUniformLocation(m_handles.shaderProgram, m_texAlbedo.samplerName.c_str()),
		0
	);

	/* MVP Uniform Block */
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffUniformMVP);

	/* Storage Buffer(s) */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffStorageBoneRefs);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffStorageBones);

	/* VAO Binding */
	glBindVertexArray(m_handles.buffRenderable.vaoHandle);
}

void pje::renderer::RendererGL::renderIn(GLFWwindow* window, const pje::engine::types::LSysObject& renderable) {
	static int prog;
	m_windowIconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);

	if (!m_windowIconified) {
		/* Clearing attachments of current active framebuffer */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Checking whether shaderProgram is still current program */
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog != m_handles.shaderProgram) 
			glUseProgram(m_handles.shaderProgram);

		/* Drawing */
		// Drawing each mesh of each primitive separately //
		for (const auto& primitive : renderable.m_objectPrimitives) {
			for (const auto& mesh : primitive.m_meshes) {
				glDrawElementsInstancedBaseVertex(
					GL_TRIANGLES,
					static_cast<GLsizei>(mesh.m_indices.size()),
					GL_UNSIGNED_INT,
					(void*)(sizeof(uint32_t) * (primitive.m_offsetPriorPrimitivesIndices + mesh.m_offsetPriorMeshesIndices)),
					m_instanceCount,
					primitive.m_offsetPriorPrimitivesVertices + mesh.m_offsetPriorMeshesVertices
				);
			}
		}

		/* Swapping buffers for double buffering */
		glfwSwapBuffers(window);
	}
	else {
		glfwWaitEvents();
	}
}

void pje::renderer::RendererGL::updateBuffer(const pje::engine::types::LSysObject& renderable, BufferType type) {
	switch (type) {
	case BufferType::UniformMVP:
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffUniformMVP);
		glBufferSubData(
			GL_UNIFORM_BUFFER, 
			0, 
			sizeof(pje::engine::types::MVPMatrices), 
			&renderable.m_matrices
		);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		break;
	case BufferType::StorageBoneRefs:
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffStorageBoneRefs);
		glBufferSubData(
			GL_SHADER_STORAGE_BUFFER, 
			0, 
			sizeof(pje::engine::types::BoneRef) * renderable.m_boneRefs.size(), 
			renderable.m_boneRefs.data()
		);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		break;
	case BufferType::StorageBones:
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffStorageBones);
		glBufferSubData(
			GL_SHADER_STORAGE_BUFFER,
			0,
			sizeof(glm::mat4) * renderable.m_bones.size(),
			renderable.getBoneMatrices().data()
		);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		break;
	}
}

/* ### Private methods ### */

void pje::renderer::RendererGL::setGlobalSettings() {
	/* Defining viewport for active framebuffer (only using default framebuffer) */
	glViewport(0, 0, m_renderWidth, m_renderHeight);

	/* Defining color/depth values for glClear() */
	glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
	glClearDepth(1.0f);

	/* Global depth state */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	/* Global MSAA state */
	if (m_msaaFactor > 1)
		glEnable(GL_MULTISAMPLE);
}

void pje::renderer::RendererGL::setShaderProgram(std::string shaderName, uint32_t& rawProgram) {
	std::string vsm = loadShader("assets/shaders/" + shaderName + ".vert");
	std::string fsm = loadShader("assets/shaders/" + shaderName + ".frag");

#ifdef DEBUG
	std::cout << "\n[DEBUG - SHADERCODE]\n" << vsm << "\n\n" << fsm << std::endl;
#endif // DEBUG

	const char* vsmSrc = vsm.c_str();
	const char* fsmSrc = fsm.c_str();

	m_handles.vertexShaderModule	= glCreateShader(GL_VERTEX_SHADER);
	m_handles.fragmentShaderModule	= glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(m_handles.vertexShaderModule, 1, &vsmSrc, NULL);
	glShaderSource(m_handles.fragmentShaderModule, 1, &fsmSrc, NULL);

	glCompileShader(m_handles.vertexShaderModule);
	glCompileShader(m_handles.fragmentShaderModule);

	int successRes;

	glGetShaderiv(m_handles.vertexShaderModule, GL_COMPILE_STATUS, &successRes);
	if (!successRes)
		throw std::runtime_error("Failed to compile vertex shader.");
	glGetShaderiv(m_handles.fragmentShaderModule, GL_COMPILE_STATUS, &successRes);
	if (!successRes)
		throw std::runtime_error("Failed to compile fragment shader.");

	rawProgram = glCreateProgram();
	glAttachShader(rawProgram, m_handles.vertexShaderModule);
	glAttachShader(rawProgram, m_handles.fragmentShaderModule);
	glLinkProgram(rawProgram);

	glGetProgramiv(rawProgram, GL_LINK_STATUS, &successRes);
	if (!successRes)
		throw std::runtime_error("Failed to link shader modules to shader program.");

	/* Cleanup for next call of setShaderProgram() */
	glDetachShader(rawProgram, m_handles.vertexShaderModule);
	glDetachShader(rawProgram, m_handles.fragmentShaderModule);
	glDeleteShader(m_handles.vertexShaderModule);
	glDeleteShader(m_handles.fragmentShaderModule);
}

std::string pje::renderer::RendererGL::loadShader(const std::string& filename) {
	std::ifstream currentFile(filename);

	// ifstream converts object currentFile to TRUE if the file was opened successfully
	if (currentFile) {
		/* stream string of currentFile into shadercode */
		std::stringstream shadercode;
		shadercode << currentFile.rdbuf();
		currentFile.close();

		return shadercode.str();
	}
	else {
		throw std::runtime_error("Failed to load a shader into RAM!");
	}
}