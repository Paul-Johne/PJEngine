#include "rendererGL.h"

pje::renderer::RendererGL::RendererGL(const pje::engine::ArgsParser& parser, GLFWwindow* const window, const pje::engine::types::LSysObject& renderable) :
	m_renderWidth(parser.m_width), m_renderHeight(parser.m_height), m_windowIconified(false), 
	m_vsync(parser.m_vsync), m_anisotropyLevel(AnisotropyLevel::TWOx), m_msaaFactor(4), m_instanceCount(parser.m_amountOfObjects) {

	glfwMakeContextCurrent(window);
	if (gl3wInit()) {
		glfwTerminate();
		throw std::runtime_error("Init of gl3w failed.");
	}
	std::cout << "[GL3W] \tOpenGL Version: " << glGetString(GL_VERSION) << "\n\t[GPU]\t" << glGetString(GL_RENDERER) << std::endl;

	// TODO(split content of setXXX into more readable submethods)
	setXXX();
}

pje::renderer::RendererGL::~RendererGL() {}

void pje::renderer::RendererGL::renderIn(GLFWwindow* window) {
	m_windowIconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
	if (!m_windowIconified) {
		/* Clearing buffer(s) */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO("per Frame logic")

		/* Swapping buffers for double buffering */
		glfwSwapBuffers(window);
	}
	else {
		glfwWaitEvents();
	}
}

void pje::renderer::RendererGL::setXXX() {
	/* Defining color for glClear() */
	glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
}