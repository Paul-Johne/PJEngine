/* Ignores header if not needed after #include */
	#pragma once

/* Third Party Files */
	#include <fstream>		// read from files

	#include <GL/gl3w.h>

/* Project Files */
	#include "../engine/rendererInterface.h"
	#include "../engine/pjeBuffers.h"

namespace pje::renderer {

	/* RendererGL - OpenGL powered renderer for demoPerformance */
	class RendererGL final : public pje::renderer::RendererInterface {

	};
}