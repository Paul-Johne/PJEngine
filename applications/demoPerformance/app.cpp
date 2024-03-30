#include "app.h"

int main() {
	std::cout << "[OS] C++ Version: " << __cplusplus << std::endl;

	int res;
	GLFWwindow* window;

	if (glfwInit() == GLFW_TRUE) {
		window = glfwCreateWindow(
			1280, 720, "bachelor", nullptr, nullptr
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

	if (res = gl3wInit()) {
		glfwTerminate();
		return res;
	}
	std::cout << "[GL3W] OpenGl Version: " << glGetString(GL_VERSION) << std::endl;

	std::cout << "Rendering.." << std::endl;
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}