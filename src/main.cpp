#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "logger.h"
#include "vulkan_base/vulkan_base.h"
#include "model.h"

#define FRAMES_IN_FLIGHT 2

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
std::vector<VkFramebuffer> framebuffers;
VulkanPipeline spritePipeline;
VulkanPipeline modelPipeline;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fences[FRAMES_IN_FLIGHT];
VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];
VulkanBuffer vertexBuffer;
VulkanBuffer indexBuffer;
VulkanImage image;

VkSampler sampler;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;
VkDescriptorSetLayout descriptorLayout;

Model model;

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

float vertexData[] = {
	0.5f, -0.5f,		// Position
	1.0f, 0.0f, 0.0f,	// Color
	1.0f, 0.0f,			// Texcoord

	0.5f, 0.5f,
	0.0f, 1.0f, 0.0f,
	1.0f, 1.0f,

	-0.5f, 0.5f,
	0.0f, 0.0f, 1.0f,
	0.0f, 1.0f,

	-0.5f, -0.5f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f,
};

uint32_t indexData[] = {
	0, 1, 2,
	3, 0, 2,
};

void initApplication(SDL_Window* window) {
	const char* additionalInstanceExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
	};
	uint32_t instanceExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, 0);
	const char** enabledInstanceExtensions = new const char* [instanceExtensionCount + ARRAY_COUNT(additionalInstanceExtensions)];
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, enabledInstanceExtensions);
	for (uint32_t i = 0; i < ARRAY_COUNT(additionalInstanceExtensions); ++i) {
		enabledInstanceExtensions[instanceExtensionCount++] = additionalInstanceExtensions[i];
	}

	const char* enabledDeviceExtensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	context = initVulkan(instanceExtensionCount, enabledInstanceExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);
	
	SDL_Vulkan_CreateSurface(window, context->instance, &surface);
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	recreateRenderPass();

	{
		VkSamplerCreateInfo createInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		createInfo.magFilter = VK_FILTER_NEAREST;
		createInfo.minFilter = VK_FILTER_NEAREST;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeV = createInfo.addressModeU;
		createInfo.addressModeW = createInfo.addressModeU;
		createInfo.mipLodBias = 0.0f;
		createInfo.maxAnisotropy = 1.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 1.0f;
		VKA(vkCreateSampler(context->device, &createInfo, 0, &sampler));
	}

	{
		int width, height, channels;
		uint8_t* data = stbi_load("../data/images/logo.png", &width, &height, &channels, 4);
		if(!data) {
			LOG_ERROR("Could not load image data");
		}
		createImage(context, &image, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		uploadDataToImage(context, &image, data, width * height * 4, width, height, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		stbi_image_free(data);
	}

	{
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		VkDescriptorPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
		createInfo.pPoolSizes = poolSizes;
		VKA(vkCreateDescriptorPool(context->device, &createInfo, 0, &descriptorPool));
	}

	{
		VkDescriptorSetLayoutBinding bindings[] = {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		};
		VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		createInfo.bindingCount = ARRAY_COUNT(bindings);
		createInfo.pBindings = bindings;
		VKA(vkCreateDescriptorSetLayout(context->device, &createInfo, 0, &descriptorLayout));

		VkDescriptorSetAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorLayout;
		VKA(vkAllocateDescriptorSets(context->device, &allocateInfo, &descriptorSet));

		VkDescriptorImageInfo imageInfo = { sampler, image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		VkWriteDescriptorSet descriptorWrites[1];
		descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].pImageInfo = &imageInfo;
		VK(vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, 0));
	}

	VkVertexInputAttributeDescription vertexAttributeDescriptions[3] = {};
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 2;
	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[2].offset = sizeof(float) * 5;
	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBinding.stride = sizeof(float) * 7;
	spritePipeline = createPipeline(context, "../shaders/texture_vert.spv", "../shaders/texture_frag.spv", renderPass, swapchain.width, swapchain.height, 
									vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding, 1, &descriptorLayout, 0);


	VkVertexInputAttributeDescription modelAttributeDescriptions[2] = {};
	modelAttributeDescriptions[0].binding = 0;
	modelAttributeDescriptions[0].location = 0;
	modelAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	modelAttributeDescriptions[0].offset = 0;
	modelAttributeDescriptions[1].binding = 0;
	modelAttributeDescriptions[1].location = 1;
	modelAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	modelAttributeDescriptions[1].offset = 0;
	VkVertexInputBindingDescription modelInputBinding = {};
	modelInputBinding.binding = 0;
	modelInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	modelInputBinding.stride = sizeof(float) * 6;
	VkPushConstantRange pushConstant = {};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(glm::mat4);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	modelPipeline = createPipeline(context, "../shaders/model_vert.spv", "../shaders/model_frag.spv", renderPass, swapchain.width, swapchain.height,
									modelAttributeDescriptions, ARRAY_COUNT(modelAttributeDescriptions), &modelInputBinding, 0, 0, &pushConstant);

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

	createBuffer(context, &vertexBuffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &vertexBuffer, vertexData, sizeof(vertexData));
	
	createBuffer(context, &indexBuffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &indexBuffer, indexData, sizeof(indexData));

	model = createModel(context, "../data/models/monkey.glb");
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

// https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
glm::mat4 getProjectionInverseZ(float fovy, float width, float height, float zNear) {
	float f = 1.0f / tanf(fovy / 2.0f);
	float aspect = width / height;
	return glm::mat4(
		f / aspect, 0.0f,  0.0f,  0.0f,
		0.0f,   -f,  0.0f,  0.0f,	// -f to flip y axis
		0.0f, 0.0f,  0.0f, 1.0f,
		0.0f, 0.0f, zNear,  0.0f
	);
}

void renderApplication() {
	static float greenChannel = 0.0f;
	static float time = 0.0f;
	time += 0.01f;
	greenChannel += 0.01f;
	if (greenChannel > 1.0f) greenChannel = 0.0f;
	uint32_t imageIndex = 0;
	static uint32_t frameIndex = 0;

	// Wait for the n-2 frame to finish to be able to reuse its acquireSemaphore in vkAcquireNextImageKHR
	VKA(vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));

	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
		return;
	} else {
		VKA(vkResetFences(context->device, 1, &fences[frameIndex]));
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

#if 0
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline.pipeline);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline.pipelineLayout, 0, 1, &descriptorSet, 0, 0);
		vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indexData), 1, 0, 0, 0);
#else
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f));
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -time, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;

		//glm::mat4 projection = glm::ortho(0.0f, (float)swapchain.width, 0.0f, (float)swapchain.height, 0.0f, 1000.0f);
		glm::mat4 projection = getProjectionInverseZ(glm::radians(80.0f), swapchain.width, swapchain.height, 0.01f);
		glm::mat4 modelViewProj = projection * modelMatrix;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipeline);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdPushConstants(commandBuffer, modelPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(modelViewProj), &modelViewProj);
		vkCmdDrawIndexed(commandBuffer, model.numIndices, 1, 0, 0, 0);
#endif

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

	VK(vkDestroyDescriptorPool(context->device, descriptorPool, 0));
	VK(vkDestroyDescriptorSetLayout(context->device, descriptorLayout, 0));

	destroyModel(context, &model);
	destroyBuffer(context, &vertexBuffer);
	destroyBuffer(context, &indexBuffer);

	destroyImage(context, &image);

	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		VK(vkDestroyFence(context->device, fences[i], 0));
		VK(vkDestroySemaphore(context->device, acquireSemaphores[i], 0));
		VK(vkDestroySemaphore(context->device, releaseSemaphores[i], 0));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VK(vkDestroyCommandPool(context->device, commandPools[i], 0));
	}

	destroyPipeline(context, &spritePipeline);
	destroyPipeline(context, &modelPipeline);

	vkDestroySampler(context->device, sampler, 0);

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