#include "app.h"

int main(int argc, char* argv[]) {
	std::cout << "[OS] \tC++ Version: " << __cplusplus << "\n";
	std::cout << "[OS] \tValue of argc: " << argc << std::endl;

	// TEMP : Variables
	GLFWwindow*									window;
	std::unique_ptr<pje::engine::ArgsParser>	parser;
	std::unique_ptr<pje::engine::LSysGenerator>	generator;
	std::unique_ptr<pje::engine::Sourceloader>	loader;
	std::unique_ptr<pje::engine::PlantTurtle>	plantTurtle;

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

		std::cout << "[PJE] \tL-System word: " << generator->getCurrentLSysWord() << std::endl;
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
	}

	// TEMP : Sourceloader Init Test
	try {
		loader = std::make_unique<pje::engine::Sourceloader>();
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

		plantTurtle->buildLSysObject(generator->getCurrentLSysWord(), loader->m_primitives);
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
		return -1;
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

	// TEMP : gl3w/vulkan Init Test
	if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
		if (gl3wInit()) {
			glfwTerminate();
			return -3;
		}
		std::cout << "[GL3W] \tOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	}
	else if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
		VkApplicationInfo appInfo = {};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext				= nullptr;
		appInfo.pApplicationName	= "bachelor";
		appInfo.applicationVersion	= VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName			= "PJE";
		appInfo.engineVersion		= VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_1;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo	= &appInfo;

		VkInstance instance;
		vkCreateInstance(&createInfo, nullptr, &instance);

		uint32_t apiVersion;
		vkEnumerateInstanceVersion(&apiVersion);

		uint32_t apiVersionMajor = VK_VERSION_MAJOR(apiVersion);
		uint32_t apiVersionMinor = VK_VERSION_MINOR(apiVersion);
		uint32_t apiVersionPatch = VK_VERSION_PATCH(apiVersion);

		std::cout << "[VK] \tVulkan Version: " << apiVersionMajor << "." << apiVersionMinor << "." << apiVersionPatch << std::endl;
		vkDestroyInstance(instance, nullptr);
	}
	else {
		std::cout << "[PJE] \tNO GRAPHICS API SET!" << std::endl;
		return -4;
	}

	std::cout << "[PJE] \tRendering.." << std::endl;
	glfwTerminate();
	return 0;
}