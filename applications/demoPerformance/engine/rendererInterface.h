/* Ignores header if not needed after #include */
	#pragma once

namespace pje::renderer {

	/* RendererInterface - provides all methods that both OpenGL and Vulkan need to fulfill for PJE */
	class RendererInterface {
	protected:
		RendererInterface();
		virtual ~RendererInterface();
	};
}