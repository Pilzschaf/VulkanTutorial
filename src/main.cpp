#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include "logger.h"
#include "vulkan_base/vulkan_base.h"

#define FRAMES_IN_FLIGHT 2

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
std::vector<VkFramebuffer> framebuffers;
VulkanPipeline pipeline;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fences[FRAMES_IN_FLIGHT];
VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];

bool handleMessage() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		}
	}
	return true;
}

void recreateRenderPass() {
	if(renderPass) {
		for (uint32_t i = 0; i < framebuffers.size(); ++i) {
			VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
		}
		destroyRenderpass(context, renderPass);
	}
	framebuffers.clear();

	renderPass = createRenderPass(context, swapchain.format);
	framebuffers.resize(swapchain.images.size());
	for (uint32_t i = 0; i < swapchain.images.size(); ++i) {
		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &swapchain.imageViews[i];
		createInfo.width = swapchain.width;
		createInfo.height = swapchain.height;
		createInfo.layers = 1;
		VKA(vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]));
	}
}

void initApplication(SDL_Window* window) {
	uint32_t instanceExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, 0);
	const char** enabledInstanceExtensions = new const char* [instanceExtensionCount];
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, enabledInstanceExtensions);

	const char* enabledDeviceExtensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	context = initVulkan(instanceExtensionCount, enabledInstanceExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);
	
	SDL_Vulkan_CreateSurface(window, context->instance, &surface);
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	recreateRenderPass();

	pipeline = createPipeline(context, "../shaders/triangle_vert.spv", "../shaders/triangle_frag.spv", renderPass, swapchain.width, swapchain.height);

	for(uint32_t i = 0; i < ARRAY_COUNT(fences); ++i) {
		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VKA(vkCreateFence(context->device, &createInfo, 0, &fences[i]));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(acquireSemaphores); ++i) {
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphores[i]));
	}

	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPools[i];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffers[i]));
	}
}

void recreateSwapchain() {
	VulkanSwapchain oldSwapchain = swapchain;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if(surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return;
	}


	VKA(vkDeviceWaitIdle(context->device));
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &oldSwapchain);

	destroySwapchain(context, &oldSwapchain);
	recreateRenderPass();
}

void renderApplication() {
	static float greenChannel = 0.0f;
	greenChannel += 0.01f;
	if (greenChannel > 1.0f) greenChannel = 0.0f;
	uint32_t imageIndex = 0;
	static uint32_t frameIndex = 0;

	// Wait for the n-2 frame to finish to be able to reuse its acquireSemaphore in vkAcquireNextImageKHR
	VKA(vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
	VKA(vkResetFences(context->device, 1, &fences[frameIndex]));

	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
		return;
	} else {
		ASSERT_VULKAN(result);
	}

	VKA(vkResetCommandPool(context->device, commandPools[frameIndex], 0));

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	{
		VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
		VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VkViewport viewport = { 0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height };
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkClearValue clearValue = {0.0f, greenChannel, 1.0f, 1.0f};
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height} };
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VKA(vkEndCommandBuffer(commandBuffer));
	}
	
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &acquireSemaphores[frameIndex];
	VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &releaseSemaphores[frameIndex];
	VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fences[frameIndex]));

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &releaseSemaphores[frameIndex];
	result = VK(vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo));
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
	} else {
		ASSERT_VULKAN(result);
	}

	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void shutdownApplication() {
	VKA(vkDeviceWaitIdle(context->device));

	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		VK(vkDestroyFence(context->device, fences[i], 0));
		VK(vkDestroySemaphore(context->device, acquireSemaphores[i], 0));
		VK(vkDestroySemaphore(context->device, releaseSemaphores[i], 0));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VK(vkDestroyCommandPool(context->device, commandPools[i], 0));
	}

	destroyPipeline(context, &pipeline);

	for (uint32_t i = 0; i < framebuffers.size(); ++i) {
		VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
	}
	framebuffers.clear();
	destroyRenderpass(context, renderPass);
	destroySwapchain(context, &swapchain);
	VK(vkDestroySurfaceKHR(context->instance, surface, 0));
	exitVulkan(context);
}

int main() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Error initializing SDL: ", SDL_GetError());
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Vulkan Tutorial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1240, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		LOG_ERROR("Error creating SDL window");
		return 1;
	}

	initApplication(window);

	while (handleMessage()) {
		renderApplication();
	}

	shutdownApplication();

	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}