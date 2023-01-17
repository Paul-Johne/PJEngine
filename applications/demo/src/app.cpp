#include <app.h>

using namespace std;

int main() {
	cout << "C++-Version: " << __cplusplus << endl;

	/* Error Handler */
	int res;
	/* 3D Objects */
	std::unique_ptr<pje::ModelLoader> modelLoader(std::make_unique<pje::ModelLoader>());
	/* Append data in modelLoader to pje::debugMeshes */
	pje::debugMeshes.insert(pje::debugMeshes.end(), modelLoader->m_models.begin(), modelLoader->m_models.end());

	try {
		pje::context.startTimePoint = std::chrono::steady_clock::now();
		cout << "[DEBUG] Current number of loaded 3d objects: " << modelLoader.get()->m_active_models << endl;
		cout << "[DEBUG] Current number of meshes in pje::debugMeshes: " << pje::debugMeshes.size() << endl;

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