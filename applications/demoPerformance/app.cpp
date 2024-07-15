#include "app.h"

/* Choose between quantity and time test: */
#if 1
	#define QUANTITY_TEST
#else
	#define TIME_TEST
#endif

#if defined(QUANTITY_TEST)
	#define PERFORMANCE_TEST_FRAMES		20000
#elif defined(TIME_TEST)
	#define PERFORMANCE_TEST_SECONDS	5
#endif

/* ######################################################################## */

int main(int argc, char* argv[]) {
	std::cout << "[OS] \tC++ Version: " << __cplusplus << "\n";
	std::cout << "[OS] \tValue of argc: " << argc << std::endl;
	
	/* API-unspecific variables */
	std::unique_ptr<pje::engine::ArgsParser>	parser;
	std::unique_ptr<pje::engine::LSysGenerator>	generator;
	std::unique_ptr<pje::engine::Sourceloader>	loader;
	std::unique_ptr<pje::engine::PlantTurtle>	plantTurtle;
	/* API-specific variables */
	GLFWwindow*									window = nullptr;

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
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);						// no resizing for both APIs

			window = glfwCreateWindow(
				parser->m_width, parser->m_height, "Bachelor", nullptr, nullptr
			);
		}

		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);				// context creation for OpenGL
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);					// double buffering
			glfwWindowHint(GLFW_SAMPLES, 4);								// default framebuffer => 4x MSAA
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);						// no resizing for both APIs

			window = glfwCreateWindow(
				parser->m_width, parser->m_height, "Bachelor", nullptr, nullptr
			);
		}
		
		/* Window creation failed */
		if (!window) {
			glfwTerminate();
			std::cout << "[PJE] \tNO API WAS FOUND TO CREATE GLFW WINDOW!" << std::endl;
			return -2;
		}
	}
	else {
		return -1;
	}

	/* Scene preparation - Renderable */
	plantTurtle->m_renderable.placeObjectInWorld(glm::vec3(0.0f), -10.0f, glm::vec3(1.0f));
	plantTurtle->m_renderable.placeCamera(glm::vec3(1.0f, 1.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	if (parser->m_graphicsAPI.find("vulkan") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / (float)parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::Vulkan
		);
	else if (parser->m_graphicsAPI.find("opengl") != std::string::npos)
		plantTurtle->m_renderable.setPerspective(
			glm::radians(60.0f), parser->m_width / (float)parser->m_height, 0.1f, 100.0f, pje::engine::types::LSysObject::API::OpenGL
		);
	plantTurtle->m_renderable.updateMVP();

	/* Scene preparation - Test specific variables */
#if defined(QUANTITY_TEST)
	uint32_t					deltaFrame = 1;
	std::vector<size_t>			renderDurations(PERFORMANCE_TEST_FRAMES);
#elif defined(TIME_TEST)
	auto						testDuration = std::chrono::seconds(PERFORMANCE_TEST_SECONDS);
	std::chrono::milliseconds	deltaTime;
	size_t						amountOfRenderedFrames = 0;
#endif

	/* Scene preparation - Start */
	auto startTime = std::chrono::steady_clock::now();

	/* API-specific part: Vulkan | OpenGL */
	try {
		/* Vulkan */
		if (parser->m_graphicsAPI.find("vulkan") != std::string::npos) {
			/* Renderer - Init */
			std::unique_ptr<pje::renderer::RendererVK> vkRenderer =
				std::make_unique<pje::renderer::RendererVK>(*parser, window, plantTurtle->m_renderable);

			/* Uploading shader resources */
			vkRenderer->uploadRenderable(plantTurtle->m_renderable);
			vkRenderer->uploadTextureOf(plantTurtle->m_renderable, true, pje::renderer::RendererVK::TextureType::Albedo);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffUniformMVP, pje::renderer::RendererVK::BufferType::UniformMVP);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBoneRefs, pje::renderer::RendererVK::BufferType::StorageBoneRefs);
			vkRenderer->uploadBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);

			/* Binding shader resources - ONCE to descriptor set */
			vkRenderer->bindToShader(vkRenderer->m_buffUniformMVP, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBoneRefs, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_buffStorageBones, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			vkRenderer->bindToShader(vkRenderer->m_texAlbedo, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			std::cout << 
				"[PJE] \tVulkan setup time: " << 
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() << 
				"ms" <<
			std::endl;

			/* Renderloop */
			auto startRenderingTime = std::chrono::steady_clock::now();
			while (!glfwWindowShouldClose(window)) {

#if defined(QUANTITY_TEST)
				plantTurtle->m_renderable.animWindBlow(deltaFrame * 1e-3, 0.5f);
				plantTurtle->m_renderable.placeCamera(glm::vec3(1.0f, 1.0f, deltaFrame * 1e-3), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				plantTurtle->m_renderable.updateMVP();
#elif defined(TIME_TEST)
				deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startRenderingTime);
				plantTurtle->m_renderable.animWindBlow(deltaTime.count() * 1e-3, 0.5f);
#endif
				/* Updating shader resources */
				vkRenderer->updateBuffer(plantTurtle->m_renderable, vkRenderer->m_buffStorageBones, pje::renderer::RendererVK::BufferType::StorageBones);
				vkRenderer->updateBuffer(plantTurtle->m_renderable, vkRenderer->m_buffUniformMVP, pje::renderer::RendererVK::BufferType::UniformMVP);

#if defined(QUANTITY_TEST)
				auto startFrameTime = std::chrono::steady_clock::now();
#endif
				/* Rendering */
				vkRenderer->renderIn(window, plantTurtle->m_renderable);
#if defined(TIME_TEST)
				++amountOfRenderedFrames;
#endif

				/* Saving performance data | Closing window after condition is met */
#if defined(QUANTITY_TEST)
				renderDurations.at(PERFORMANCE_TEST_FRAMES - deltaFrame) = 
					std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::steady_clock::now() - startFrameTime
					).count();

				if (deltaFrame < PERFORMANCE_TEST_FRAMES)
					++deltaFrame;
				else
					glfwSetWindowShouldClose(window, GLFW_TRUE);
#elif defined(TIME_TEST)
				if (deltaTime >= testDuration)
					glfwSetWindowShouldClose(window, GLFW_TRUE);
#endif
				glfwPollEvents();
			}
		}

		/* OpenGL */
		else if (parser->m_graphicsAPI.find("opengl") != std::string::npos) {
			/* Renderer - Init */
			std::unique_ptr<pje::renderer::RendererGL> glRenderer = 
				std::make_unique<pje::renderer::RendererGL>(*parser, window, plantTurtle->m_renderable);

			/* Uploading shader resources */
			glRenderer->uploadRenderable(plantTurtle->m_renderable);
			glRenderer->uploadTextureOf(plantTurtle->m_renderable, true, pje::renderer::RendererGL::TextureType::Albedo);
			glRenderer->uploadBuffer(plantTurtle->m_renderable, pje::renderer::RendererGL::BufferType::UniformMVP);
			glRenderer->uploadBuffer(plantTurtle->m_renderable, pje::renderer::RendererGL::BufferType::StorageBoneRefs);
			glRenderer->uploadBuffer(plantTurtle->m_renderable, pje::renderer::RendererGL::BufferType::StorageBones);

			/* Binding shader resources */
			glRenderer->bindRenderable(plantTurtle->m_renderable);

			std::cout <<
				"[PJE] \tOpenGL setup time: " <<
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() <<
				"ms" <<
			std::endl;

			/* Renderloop */
			auto startRenderingTime = std::chrono::steady_clock::now();
			while (!glfwWindowShouldClose(window)) {

#if defined(QUANTITY_TEST)
				plantTurtle->m_renderable.animWindBlow(deltaFrame * 1e-3, 0.5f);
				plantTurtle->m_renderable.placeCamera(glm::vec3(1.0f, 1.0f, deltaFrame * 1e-3), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				plantTurtle->m_renderable.updateMVP();
#elif defined(TIME_TEST)
				deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startRenderingTime);
				plantTurtle->m_renderable.animWindBlow(deltaTime.count() * 1e-3, 0.5f);
#endif
				/* Updating shader resources */
				glRenderer->updateBuffer(plantTurtle->m_renderable, pje::renderer::RendererGL::BufferType::StorageBones);
				glRenderer->updateBuffer(plantTurtle->m_renderable, pje::renderer::RendererGL::BufferType::UniformMVP);

#if defined(QUANTITY_TEST)
				auto startFrameTime = std::chrono::steady_clock::now();
#endif
				/* Rendering */
				glRenderer->renderIn(window, plantTurtle->m_renderable);
#if defined(TIME_TEST)
				++amountOfRenderedFrames;
#endif

				/* Saving performance data | Closing window after condition is met */
#if defined(QUANTITY_TEST)
				renderDurations.at(PERFORMANCE_TEST_FRAMES - deltaFrame) =
					std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::steady_clock::now() - startFrameTime
					).count();

				if (deltaFrame < PERFORMANCE_TEST_FRAMES)
					++deltaFrame;
				else
					glfwSetWindowShouldClose(window, GLFW_TRUE);
#elif defined(TIME_TEST)
				if (deltaTime >= testDuration)
					glfwSetWindowShouldClose(window, GLFW_TRUE);
#endif
				glfwPollEvents();
			}
		}

		/* Unknown environment */
		else {
			std::cout << "[PJE] \tNO GRAPHICS API WERE SELECTED!" << std::endl;
			if (window)
				glfwTerminate();
			return -3;
		}
	}
	catch (std::runtime_error& ex) {
		std::cout << "[ERROR] Exception thrown: " << ex.what() << std::endl;
	}
	
	/* Printing performance data */
#if defined(QUANTITY_TEST)
	auto max	= *std::max_element(renderDurations.begin(), renderDurations.end());
	auto min	= *std::min_element(renderDurations.begin(), renderDurations.end());
	auto median = getMedian(renderDurations);
	auto mean	= std::accumulate(renderDurations.begin(), renderDurations.end(), 0.0) / renderDurations.size();
	auto sd		= getStandardDeviation(renderDurations, mean);

	std::cout << 
		"[PJE] \tFrametime Results:\n\tmin (" << 
		min << "us) | max (" << max << "us) | median (" << 
		median << "us) | mean (" << mean << "us) | standard deviation (" << sd << "us)" <<
	std::endl;
#elif defined(TIME_TEST)
	std::cout << "[PJE] \tFrames rendered: " << amountOfRenderedFrames << std::endl;
#endif

	/* Terminating application */
	glfwTerminate();
	std::cout << "[PJE] \tPerformance test completed!" << std::endl;
	std::cout << "\nPress ENTER to close this app..";
	std::cin.get();
	return 0;
}