#include <app.h>

using namespace std;

int main() {
	cout << "C++-Version: " << __cplusplus << endl;

	int res;

	try {
		//pje::modelLoader.get()->loadModel("some/wrong/data");

		pje::context.startTimePoint = std::chrono::steady_clock::now();

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
		return -1;
	}

	return 0;
}