/* Ignores header if not needed after #include */
	#pragma once

	#include <iostream>
	#include <limits>

	#include "imgui.h"
	#include "imgui_impl_glfw.h"
	#include "imgui_impl_vulkan.h"

	#include <globalParams.h>

namespace pje {

	/* #### ImGui Implementation in PJEngine #### */
	class EngineGui {
	public:
		ImGuiIO				m_io;
		VkDescriptorPool	m_descriptorPool;
		VkRenderPass		m_renderpass;

		EngineGui();
		~EngineGui();

		static void handleVkImguiError(VkResult res);

		void init(GLFWwindow* window);
		void nextGuiFrame();
		void drawGui(uint32_t swapchainImageIndex);
		void shutdown();
	};
}