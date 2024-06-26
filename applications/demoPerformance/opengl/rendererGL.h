/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <cstdint>		// fixed size integer
	#include <fstream>		// read from files
	#include <iostream>		// i/o stream

	#include <GL/gl3w.h>
	#include <GLFW/glfw3.h>

/* Project Files */
	#include "../engine/argsParser.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	/* RendererGL - OpenGL powered renderer for demoPerformance */
	class RendererGL final {
	public:
		RendererGL() = delete;
		RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable);
		~RendererGL();

		/** Methods for app.cpp **/
		void renderIn(GLFWwindow* window);

	private:
		enum class AnisotropyLevel { Disabled, TWOx, FOURx, EIGHTx, SIXTEENx };

		uint16_t			m_renderWidth;
		uint16_t			m_renderHeight;
		bool				m_windowIconified;
		bool				m_vsync;
		AnisotropyLevel		m_anisotropyLevel;
		uint8_t				m_msaaFactor;
		uint8_t				m_instanceCount;

		void setXXX();
	};
}