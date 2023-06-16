#include <app.h>

using namespace std;

int main() {
	cout << "[OS] C++-Version: " << __cplusplus << endl;

	/* Error Handler */
	int res;

	/* 3D Objects Loader - Loads all models in assets during construction */
	std::unique_ptr<pje::Modelloader> modelLoader(std::make_unique<pje::Modelloader>());

	/* Append debugMesh and pje::modelLoader.m_models to loadedModels */
	pje::loadedModels.insert(pje::loadedModels.end(), modelLoader->m_models.begin(), modelLoader->m_models.end());
	modelLoader->m_models.clear();

#ifdef ACTIVATE_ENGINE_GUI
	std::unique_ptr<pje::EngineGui> gui(std::make_unique<pje::EngineGui>());
#endif

	try {
		pje::context.startTimePoint = std::chrono::steady_clock::now();
		cout << "[DEBUG] Current number of models in Modelloader: " << modelLoader->m_models.size() << endl;
		cout << "[DEBUG] Current number of loaded 3d objects: " << pje::loadedModels.size() << endl;

		res = pje::startGlfw3(pje::config::appName);
		if (res != 0) {
			return res;
		}

		res = pje::startVulkan();
		if (res != 0) {
			pje::stopGlfw3();
			return res;
		}

#ifdef ACTIVATE_ENGINE_GUI
		gui->init(pje::context.window);
		pje::loopVisualizationOf(pje::context.window, gui);
		gui->shutdown();
#endif

#ifndef ACTIVATE_ENGINE_GUI
		pje::loopVisualizationOf(pje::context.window);
#endif

		pje::stopVulkan();
		pje::stopGlfw3();
	}
	catch (exception& ex) {
		cout << "[ERROR] Exception thrown: " << ex.what() << endl;
		pje::cleanupRealtimeRendering();
		return -1;
	}

	return 0;
}