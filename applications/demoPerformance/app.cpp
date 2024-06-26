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
		/* Vulkan */
		if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
			/* No window context is allowed for Vulkan since it's managed manually by design */
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}
		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
			if (!parser->m_vsync)
				/* Expecting vsync as default in OpenGL/GLFW => changing vsync to false */
				glfwSwapInterval(0);
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

	/* Scene preparation */
	plantTurtle->m_renderable.placeObjectInWorld(glm::vec3(0.0f), -10.0f, glm::vec3(1.0f));
	plantTurtle->m_renderable.placeCamera(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	if (parser->m_graphicsAPI.find("vulkan") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::Vulkan
		);
	else if (parser->m_graphicsAPI.find("opengl") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::OpenGL
		);
	plantTurtle->m_renderable.updateMVP();
	auto startTime = std::chrono::steady_clock::now();

	/* API-specific part: Vulkan/OpenGL */
	try {
		/* Vulkan */
		if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
			std::unique_ptr<pje::renderer::RendererVK> vkRenderer =
				std::make_unique<pje::renderer::RendererVK>(*parser, window, plantTurtle->m_renderable);

			/* Engine data --> Vulkan buffer for shaders */
			vkRenderer->uploadRenderable(plantTurtle->m_renderable);
			vkRenderer->uploadTextureOf(plantTurtle->m_renderable, false, pje::renderer::RendererVK::TextureType::Albedo);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffUniformMVP, pje::renderer::RendererVK::BufferType::UniformMVP);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBoneRefs, pje::renderer::RendererVK::BufferType::StorageBoneRefs);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);

			/* Binding shader resources */
			vkRenderer->bindToShader(vkRenderer->m_buffUniformMVP, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBoneRefs, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBones, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_texAlbedo, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			/* Renderloop */
			while (!glfwWindowShouldClose(window)) {
				auto deltaTime =
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() / 1000.0f;

				/* Updating shader resources */
				plantTurtle->m_renderable.animWindBlow(deltaTime, 0.5f);
				vkRenderer->updateBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);

				vkRenderer->renderIn(window, plantTurtle->m_renderable);
				glfwPollEvents();
			}
		}
		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			if (gl3wInit()) {
				glfwTerminate();
				return -4;
			}
			std::unique_ptr<pje::renderer::RendererGL> glRenderer = 
				std::make_unique<pje::renderer::RendererGL>(*parser, window, plantTurtle->m_renderable);

			/* Renderloop */
			while (!glfwWindowShouldClose(window)) {
				auto deltaTime = 
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() / 1000.0f;

				/* Updating shader resources */
				plantTurtle->m_renderable.animWindBlow(deltaTime, 0.5f);

				glRenderer->renderIn(window);
				glfwPollEvents();
			}
		}
		/* Unknown environment */
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