#include "app.h"
#define PERFORMANCE_TEST_SECONDS 15

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
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);					// no glfw context creation required for Vulkan
		}

		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);				// context creation for OpenGL
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);					// double buffering
			if (!parser->m_vsync)											// vsync
				glfwSwapInterval(0);
			else
				glfwSwapInterval(1);
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

	/* Scene preparation - Renderable */
	plantTurtle->m_renderable.placeObjectInWorld(glm::vec3(0.0f), -10.0f, glm::vec3(1.0f));
	plantTurtle->m_renderable.placeCamera(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	if (parser->m_graphicsAPI.find("vulkan") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / (float)parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::Vulkan
		);
	else if (parser->m_graphicsAPI.find("opengl") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / (float)parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::OpenGL
		);
	plantTurtle->m_renderable.updateMVP();
	
	/* Scene preparation - Time variables */
	auto testDuration	= std::chrono::seconds(PERFORMANCE_TEST_SECONDS);
	auto startTime		= std::chrono::steady_clock::now();

	/* API-specific part: Vulkan | OpenGL */
	try {
		/* Vulkan */
		if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
			std::unique_ptr<pje::renderer::RendererVK> vkRenderer =
				std::make_unique<pje::renderer::RendererVK>(*parser, window, plantTurtle->m_renderable);

			/* Engine data --> Vulkan buffer for shaders */
			vkRenderer->uploadRenderable(plantTurtle->m_renderable);
			vkRenderer->uploadTextureOf(plantTurtle->m_renderable, true, pje::renderer::RendererVK::TextureType::Albedo);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffUniformMVP, pje::renderer::RendererVK::BufferType::UniformMVP);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBoneRefs, pje::renderer::RendererVK::BufferType::StorageBoneRefs);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);

			/* Binding shader resources */
			vkRenderer->bindToShader(vkRenderer->m_buffUniformMVP, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBoneRefs, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBones, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_texAlbedo, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			std::cout << 
				"[PJE] \tVulkan setup time:\t" << 
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() << 
				"ms" <<
			std::endl;

			/* Renderloop */
			auto startRenderingTime = std::chrono::steady_clock::now();
			while (!glfwWindowShouldClose(window)) {
				auto deltaTime = 
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startRenderingTime);

				/* Updating shader resources */
				plantTurtle->m_renderable.animWindBlow(deltaTime.count() * 1e-3, 0.5f);
				vkRenderer->updateBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);

				vkRenderer->renderIn(window, plantTurtle->m_renderable);
				glfwPollEvents();

				/* Close window after reaching testDuration */
				if (deltaTime >= testDuration)
					glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
		}

		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			std::unique_ptr<pje::renderer::RendererGL> glRenderer = 
				std::make_unique<pje::renderer::RendererGL>(*parser, window, plantTurtle->m_renderable);

			std::cout <<
				"[PJE] \tOpenGL setup time:\t" <<
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() <<
				"ms" <<
			std::endl;

			/* Renderloop */
			auto startRenderingTime = std::chrono::steady_clock::now();
			while (!glfwWindowShouldClose(window)) {
				auto deltaTime =
					std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startRenderingTime);

				/* Updating shader resources */
				plantTurtle->m_renderable.animWindBlow(deltaTime.count() * 1e-3, 0.5f);

				glRenderer->renderIn(window);
				glfwPollEvents();

				/* Close window after reaching testDuration */
				if (deltaTime >= testDuration)
					glfwSetWindowShouldClose(window, GLFW_TRUE);
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