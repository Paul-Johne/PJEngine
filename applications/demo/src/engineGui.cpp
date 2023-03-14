#include <engineGui.h>

/* ImGui_ImplVulkanH_XXX are optional helpers for Vulkan "backend" */

pje::EngineGui::EngineGui() : m_io{}, m_descriptorPool{ VK_NULL_HANDLE }, m_renderpass{ VK_NULL_HANDLE } {}

pje::EngineGui::~EngineGui() {}

void pje::EngineGui::handleVkImguiError(VkResult res) {
	if (res == 0)
		return;
	std::cout << "[Error] Imgui causes VkResult: \t" << res << std::endl;
	if (res < 0) {
		throw std::runtime_error("Error at EngineGui");
	}
}

/* Setup before ImGui can be used */
void pje::EngineGui::init(GLFWwindow* window) {
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	/* ... */
	this->m_io = ImGui::GetIO();

	/* VkDescriptorPool for Imgui */
	auto poolSizes = std::array{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets		= 1000;
	poolInfo.poolSizeCount	= poolSizes.size();
	poolInfo.pPoolSizes		= poolSizes.data();
	vkCreateDescriptorPool(pje::context.logicalDevice, &poolInfo, nullptr, &this->m_descriptorPool);

	/* Guess : ImGui needs its own subpass */
	ImGui_ImplGlfw_InitForVulkan(window, false);
	ImGui_ImplVulkan_InitInfo imInfo{};
	imInfo.Instance			= pje::context.vulkanInstance;
	imInfo.PhysicalDevice	= pje::context.physicalDevices[pje::context.choosenPhysicalDevice];
	imInfo.Device			= pje::context.logicalDevice;
	imInfo.QueueFamily		= pje::context.choosenQueueFamily;
	imInfo.Queue			= pje::context.queueForPrototyping;
	imInfo.PipelineCache	= VK_NULL_HANDLE;
	imInfo.DescriptorPool	= this->m_descriptorPool;
	imInfo.Subpass			= 0;
	imInfo.MinImageCount	= pje::context.numberOfImagesInSwapchain;
	imInfo.ImageCount		= pje::context.numberOfImagesInSwapchain;
	imInfo.MSAASamples		= pje::config::msaaFactor;
	imInfo.Allocator		= nullptr;
	imInfo.CheckVkResultFn	= EngineGui::handleVkImguiError;
	ImGui_ImplVulkan_Init(&imInfo, pje::context.renderPass);

	/* Building Font Atlas via VkCommandBuffer */
	static VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if (commandBuffer == VK_NULL_HANDLE) {
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext					= nullptr;
		commandBufferAllocateInfo.commandPool			= pje::context.commandPool;
		commandBufferAllocateInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount	= 1;
		vkAllocateCommandBuffers(pje::context.logicalDevice, &commandBufferAllocateInfo, &commandBuffer);
	}
	else {
		vkResetCommandBuffer(commandBuffer, 0);
	}
	
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext			= nullptr;
	commandBufferBeginInfo.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo	= nullptr;

	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &commandBuffer;

	vkQueueSubmit(pje::context.queueForPrototyping, 1, &submitInfo, pje::context.fenceBasic);
	vkWaitForFences(pje::context.logicalDevice, 1, &pje::context.fenceBasic, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(pje::context.logicalDevice, 1, &pje::context.fenceBasic);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

/* Has to be invoked to update ImGui data before rendering */
void pje::EngineGui::nextGuiFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGui::ShowDemoWindow(); // => requires imgui_demo.cpp

	ImGui::Begin("EngineGui");
	ImGui::Text("How am I supposed to change the camera position?");
	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / this->m_io.Framerate, this->m_io.Framerate);
	ImGui::End();
}

/* Has to be invoked once for each frame | each frame MUST have a new VkCommandBuffer recording */
void pje::EngineGui::drawGui(uint32_t swapchainImageIndex) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pje::context.commandBuffers[swapchainImageIndex]); // TODO (unique pipeline ?!)
}

/* Cleanup needed when EngineGui::init() was invoked */
void pje::EngineGui::shutdown() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	vkDestroyDescriptorPool(pje::context.logicalDevice, this->m_descriptorPool, nullptr);
	ImGui::DestroyContext();
}