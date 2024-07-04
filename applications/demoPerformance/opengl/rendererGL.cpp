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
}

pje::renderer::RendererGL::~RendererGL() {
	// Cleanup of OpenGL ressources (optional)
	glDeleteProgram(m_handles.shaderProgram);
}

void pje::renderer::RendererGL::uploadRenderable(const pje::engine::types::LSysObject& renderable) {
	// TODO
}

void pje::renderer::RendererGL::uploadTextureOf(const pje::engine::types::LSysObject& renderable, bool genMipmaps, TextureType type) {
	// TODO (als Erstes bearbeiten!)
}

void pje::renderer::RendererGL::uploadBuffer(const pje::engine::types::LSysObject& renderable, BufferType type) {
	// TODO
}

void pje::renderer::RendererGL::renderIn(GLFWwindow* window) {
	static int prog;
	m_windowIconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);

	if (!m_windowIconified) {
		/* Clearing attachments of current active framebuffer */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Selecting shader program for draw commands */
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog != m_handles.shaderProgram) 
			glUseProgram(m_handles.shaderProgram);

		/* Swapping buffers for double buffering */
		glfwSwapBuffers(window);
	}
	else {
		glfwWaitEvents();
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