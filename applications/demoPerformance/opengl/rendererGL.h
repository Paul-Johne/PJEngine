/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>		// fixed size integer
	#include <string>		// std::string
	#include <vector>		// std::vector
	#include <fstream>		// read from files
	#include <sstream>		// parsing input (shader code)
	#include <iostream>		// i/o stream

	#include <GL/gl3w.h>
	#include <GLFW/glfw3.h>

/* Project Files */
	#include "../engine/argsParser.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	/* HandlesGL - collection of all handle variables needed for 1 RendererGL */
	struct HandlesGL {
		uint32_t				vertexShaderModule;
		uint32_t				fragmentShaderModule;
		uint32_t				shaderProgram;

		// <Using default framebuffer> 
		// default fbo := depth + msaa + resolve/back buffer
	};

	/* RendererGL - OpenGL powered renderer for demoPerformance */
	class RendererGL final {
	public:
		enum class TextureType	{ Albedo };
		enum class BufferType	{ UniformMVP, StorageBoneRefs, StorageBones };

		RendererGL() = delete;
		RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable);
		~RendererGL();

		/** Methods for app.cpp **/
		/* 1/4: Uploading */
		void uploadRenderable(const pje::engine::types::LSysObject& renderable);
		void uploadTextureOf(const pje::engine::types::LSysObject& renderable, bool genMipmaps, TextureType type);
		void uploadBuffer(const pje::engine::types::LSysObject& renderable, BufferType type);
		/* 2/4: Binding shader resources => MIGHT BE DIFFERENT THAN Vulkan Binding !! */
		// TODO
		/* 3/4: Rendering */
		void renderIn(GLFWwindow* window);
		/* 4/4: Updating shader resources */
		// TODO

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