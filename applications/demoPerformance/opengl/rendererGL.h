/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>		// fixed size integer
	#include <string>		// std::string
	#include <vector>		// std::vector
	#include <limits>		// std::numeric_limits
	#include <fstream>		// read from files
	#include <sstream>		// parsing input (shader code)
	#include <iostream>		// i/o stream

	#include <GL/gl3w.h>
	#include <GLFW/glfw3.h>

/* Project Files */
	#include "../engine/argsParser.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	struct BufferRenderableGL {
		uint32_t			vaoHandle;
		uint32_t			verticesHandle;
		signed long long	verticesSize;
		uint32_t			indicesHandle;
		signed long long	indicesSize;

		~BufferRenderableGL() {
			glDeleteBuffers(1, &indicesHandle);
			glDeleteBuffers(1, &verticesHandle);
			glDeleteVertexArrays(1, &vaoHandle);
		}
	};

	struct ImageGL {
		uint32_t		handle;
		std::string		samplerName;

		~ImageGL() {
			glDeleteTextures(1, &handle);
		}
	};

	/* HandlesGL - collection of all handle variables needed for 1 RendererGL */
	struct HandlesGL {
		uint32_t				vertexShaderModule;
		uint32_t				fragmentShaderModule;
		uint32_t				shaderProgram;

		BufferRenderableGL		buffRenderable;
	};

	/* RendererGL - OpenGL powered renderer for demoPerformance */
	class RendererGL final {
	public:
		enum class TextureType	{ Albedo };
		enum class BufferType	{ UniformMVP, StorageBoneRefs, StorageBones };

		ImageGL		m_texAlbedo;
		uint32_t	m_buffUniformMVP;
		uint32_t	m_buffStorageBoneRefs;
		uint32_t	m_buffStorageBones;

		RendererGL() = delete;
		RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable);
		~RendererGL();

		/** Methods for app.cpp **/
		/* 1/4: Uploading */
		void uploadRenderable(const pje::engine::types::LSysObject& renderable);
		void uploadTextureOf(const pje::engine::types::LSysObject& renderable, bool genMipmaps, TextureType type);
		void uploadBuffer(const pje::engine::types::LSysObject& renderable, BufferType type);
		/* 2/4: Binding shader resources => MIGHT BE DIFFERENT THAN Vulkan Binding !! */
		void bindRenderable(const pje::engine::types::LSysObject& renderable);
		/* 3/4: Rendering */
		void renderIn(GLFWwindow* window, const pje::engine::types::LSysObject& renderable);
		/* 4/4: Updating shader resources */
		void updateBuffer(const pje::engine::types::LSysObject& renderable, BufferType type);

	private:
		enum class AnisotropyLevel { Disabled, TWOx, FOURx, EIGHTx, SIXTEENx };

		HandlesGL			m_handles;
		uint16_t			m_renderWidth;
		uint16_t			m_renderHeight;
		bool				m_windowIconified;
		bool				m_vsync;
		AnisotropyLevel		m_anisotropyLevel;
		uint8_t				m_msaaFactor;
		uint8_t				m_instanceCount;

		void setGlobalSettings();
		void setShaderProgram(std::string shaderName, uint32_t& rawProgram);
		std::string loadShader(const std::string& filename);
	};
}