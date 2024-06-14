#include "rendererGL.h"

pje::renderer::RendererGL::RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window) :
	RendererInterface(), m_renderWidth(parser.m_width), m_renderHeight(parser.m_height), m_vsync(parser.m_vsync) {

	glfwMakeContextCurrent(window);
	std::cout << "[GL3W] \tOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
}

pje::renderer::RendererGL::~RendererGL() {}