#include "app.h"

int main(int argc, char* argv[]) {
	std::cout << "[OS] \tC++ Version: " << __cplusplus << "\n";
	std::cout << "[OS] \tValue of argc: " << argc << std::endl;

	// TEMP : Variables
	GLFWwindow* window;
	std::unique_ptr<pje::engine::ArgsParser> parser;
	std::unique_ptr<pje::engine::LSysGenerator> generator;
	std::unique_ptr<pje::engine::PlantTurtle> plantTurtle;

	// TEMP : ArgsParser Init Test
	try {
		parser = std::make_unique<pje::engine::ArgsParser>(argc, argv);
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	// TEMP : LSysGenerator Init Test (1L-System)
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

		std::cout << "[PJE] \tL-System: " << generator.get()->getCurrentLSysWord() << std::endl;
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	// TEMP : TurtleInterpreter Init Test
	try {
		plantTurtle = std::make_unique<pje::engine::PlantTurtle>(
			std::string(generator->getAlphabet())
		);
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
	}

	// TEMP : GLFW Init Test
	if (glfwInit() == GLFW_TRUE) {
		window = glfwCreateWindow(
			parser->m_width, parser->m_height, "bachelor", nullptr, nullptr
		);
		if (window == NULL) {
			glfwTerminate();
			return -2;
		}
	}
	else {
		return -1;
	}
	glfwMakeContextCurrent(window);

	// TEMP : gl3w Init Test
	if (gl3wInit()) {
		glfwTerminate();
		return -3;
	}
	std::cout << "[GL3W] \tOpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	// TEMP : Renderloop Dummy
	std::cout << "[PJE] \tRendering.." << std::endl;
	glfwTerminate();
	return 0;
}