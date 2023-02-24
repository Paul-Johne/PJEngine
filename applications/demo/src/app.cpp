﻿#include <app.h>

using namespace std;

int main() {
	cout << "C++-Version: " << __cplusplus << endl;

	/* Error Handler */
	int res;
	/* 3D Objects Loader */
	std::unique_ptr<pje::ModelLoader> modelLoader(std::make_unique<pje::ModelLoader>());
	/* Append debugMesh and pje::modelLoader.m_models to loadedModels */
	pje::loadedModels.insert(pje::loadedModels.end(), modelLoader->m_models.begin(), modelLoader->m_models.end());
	modelLoader->m_models.clear();

	for (uint32_t m = 0; m < pje::loadedModels.size(); m++) {
		for (auto const mesh : pje::loadedModels[m].meshes) {
			cout << "[TEMP] OFFSETS of PJModel No." << m << " : " << mesh.m_offsetToPriorMeshes << endl;
		}
	}

	try {
		pje::context.startTimePoint = std::chrono::steady_clock::now();
		cout << "[DEBUG] Current number of models in ModelLoader: " << modelLoader->m_activeModels << endl;
		cout << "[DEBUG] Current number of loaded 3d objects: " << pje::loadedModels.size() << endl;
		cout << "[DEBUG] Current number of meshes in pje::debugMesh: " << pje::debugMesh.size() << endl;

		res = pje::startGlfw3(pje::context.appName);
		if (res != 0) {
			return res;
		}

		res = pje::startVulkan();
		if (res != 0) {
			pje::stopGlfw3();
			return res;
		}

		pje::loopVisualizationOf(pje::context.window);

		pje::stopVulkan();
		pje::stopGlfw3();
	}
	catch (exception& ex) {
		cout << "Exception thrown: " << ex.what() << endl;
		pje::cleanupRealtimeRendering();
		return -1;
	}

	return 0;
}