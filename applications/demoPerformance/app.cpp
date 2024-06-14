#include "app.h"

int main(int argc, char* argv[]) {
	std::cout << "[OS] \tC++ Version: " << __cplusplus << "\n";
	std::cout << "[OS] \tValue of argc: " << argc << std::endl;

	/* API-unspecific variables */
	std::unique_ptr<pje::engine::ArgsParser>	parser;
	std::unique_ptr<pje::engine::LSysGenerator>	generator;
	std::unique_ptr<pje::engine::Sourceloader>	loader;
	std::unique_ptr<pje::engine::PlantTurtle>	plantTurtle;
	/* API-specific variables */
	GLFWwindow*									window;

	/* Argument-Parser */
	try {
		parser = std::make_unique<pje::engine::ArgsParser>(argc, argv);
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	/* L-System Word Generator */
	try {
		generator = std::make_unique<pje::engine::LSysGenerator>(
			std::string("SLF-+[]"),							// Alphabet
			"S",											// Axiom
			std::unordered_map<std::string, std::string> {	// Rules
				{"]S","S[-S]S[+L]S"}, {"SS","S"}, 
				{"-S","SS[-L]+L"}, {"+S","S[-L]S"}, 
				{"-L","F"}, {"+L","S[-L]+L"}
			}, 
			parser->m_complexityOfObjects,					// Iterations
			"]"												// Environmental Input
		);

		std::cout << "[PJE] \tL-System word: " << generator->getCurrentLSysWord() << "\n" << std::endl;
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	/* Primitive Loader */
	try {
		loader = std::make_unique<pje::engine::Sourceloader>();
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	/* Creating LSysObject (Renderable) */
	try {
		plantTurtle = std::make_unique<pje::engine::PlantTurtle>(
			std::string(generator->getAlphabet())
		);

		plantTurtle->buildLSysObject(generator->getCurrentLSysWord(), loader->m_primitives);
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	/* GLFW window */
	if (glfwInit() == GLFW_TRUE) {
		/* No window context is allowed for Vulkan since it's managed manually by design */
		if (parser->m_graphicsAPI.find("vulkan") != std::string::npos)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
		}
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(
			parser->m_width, parser->m_height, "Bachelor", nullptr, nullptr
		);
		if (window == NULL) {
			glfwTerminate();
			return -2;
		}
	}
	else {
		return -1;
	}

	/* API-specific part: Vulkan/OpenGL */
	try {
		if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			if (gl3wInit()) {
				glfwTerminate();
				return -4;
			}
			std::unique_ptr<pje::renderer::RendererGL> glRenderer = 
				std::make_unique<pje::renderer::RendererGL>(*parser, window);
		}
		else if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
			std::unique_ptr<pje::renderer::RendererVK> vkRenderer = 
				std::make_unique<pje::renderer::RendererVK>(*parser, window);
		}
		else {
			std::cout << "[PJE] \tNO GRAPHICS API WERE SELECTED!" << std::endl;
			glfwTerminate();
			return -3;
		}
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
	}

	/* Destroying and terminating GLFW window */
	glfwTerminate();

	std::cout << "[PJE] \tPerformance test completed!" << std::endl;
	return 0;
}