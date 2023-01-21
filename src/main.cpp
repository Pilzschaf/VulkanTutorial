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

#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>

#define FRAMES_IN_FLIGHT 2

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
std::vector<VulkanImage> depthBuffers;
std::vector<VulkanImage> colorBuffers;
std::vector<VkFramebuffer> framebuffers;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fences[FRAMES_IN_FLIGHT];
VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];

VulkanBuffer spriteVertexBuffer;
VulkanBuffer spriteIndexBuffer;
VulkanImage image;
VkSampler sampler;
VkDescriptorPool spriteDescriptorPool;
VkDescriptorSet spriteDescriptorSet;
VkDescriptorSetLayout spriteDescriptorLayout;
VulkanPipeline spritePipeline;

Model model;
VulkanPipeline modelPipeline;
VkDescriptorSetLayout modelDescriptorSetLayout;
VkDescriptorPool modelDescriptorPool;
VkDescriptorSet modelDescriptorSets[FRAMES_IN_FLIGHT];
uint64_t singleElementSize;
VulkanBuffer modelUniformBuffers[FRAMES_IN_FLIGHT];

VkQueryPool timestampQueryPools[FRAMES_IN_FLIGHT];

VkDescriptorPool imguiDescriptorPool;

struct Camera {
	glm::vec3 cameraPosition;
	glm::vec3 cameraDirection;
	glm::vec3 up;
	float yaw;
	float pitch;
	glm::mat4 viewProj;
	glm::mat4 view;
	glm::mat4 proj;
} camera;

bool handleMessage() {
	ImGuiIO& io = ImGui::GetIO();

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			return false;
		case SDL_KEYDOWN:
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && !io.WantCaptureKeyboard) {
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
		case SDL_MOUSEBUTTONDOWN:
			if(event.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			break;
		}
	}
	return true;
}

void recreateRenderPass() {
	if(renderPass) {
		for (uint32_t i = 0; i < framebuffers.size(); ++i) {
			VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
		}
		for(uint32_t i = 0; i < depthBuffers.size(); ++i) {
			destroyImage(context, &depthBuffers[i]);
		}
		for(uint32_t i = 0; i < colorBuffers.size(); ++i) {
			destroyImage(context, &colorBuffers[i]);
		}
		destroyRenderpass(context, renderPass);
	}
	framebuffers.clear();
	depthBuffers.clear();
	colorBuffers.clear();

	renderPass = createRenderPass(context, swapchain.format, VK_SAMPLE_COUNT_4_BIT);
	framebuffers.resize(swapchain.images.size());
	depthBuffers.resize(swapchain.images.size());
	colorBuffers.resize(swapchain.images.size());
	for (uint32_t i = 0; i < swapchain.images.size(); ++i) {
		createImage(context, &depthBuffers.data()[i], swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_4_BIT);
		createImage(context, &colorBuffers.data()[i], swapchain.width, swapchain.height, swapchain.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_4_BIT);
		VkImageView attachments[] = {
			colorBuffers[i].view,
			depthBuffers[i].view,
			swapchain.imageViews[i],
		};
		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = ARRAY_COUNT(attachments);
		createInfo.pAttachments = attachments;
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

	//model = createModel(context, "../data/models/monkey.glb");
	model = createModel(context, "../libs/glTF-Sample-Models/2.0/BoomBox/glTF-Binary/BoomBox.glb");

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
		VKA(vkCreateDescriptorPool(context->device, &createInfo, 0, &spriteDescriptorPool));
	}

	{
		VkDescriptorSetLayoutBinding bindings[] = {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		};
		VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		createInfo.bindingCount = ARRAY_COUNT(bindings);
		createInfo.pBindings = bindings;
		VKA(vkCreateDescriptorSetLayout(context->device, &createInfo, 0, &spriteDescriptorLayout));

		VkDescriptorSetAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocateInfo.descriptorPool = spriteDescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &spriteDescriptorLayout;
		VKA(vkAllocateDescriptorSets(context->device, &allocateInfo, &spriteDescriptorSet));

		VkDescriptorImageInfo imageInfo = { sampler, image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		VkWriteDescriptorSet descriptorWrites[1];
		descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		descriptorWrites[0].dstSet = spriteDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].pImageInfo = &imageInfo;
		VK(vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, 0));
	}

	{
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, FRAMES_IN_FLIGHT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT},
		};
		VkDescriptorPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		createInfo.maxSets = FRAMES_IN_FLIGHT;
		createInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
		createInfo.pPoolSizes = poolSizes;
		VKA(vkCreateDescriptorPool(context->device, &createInfo, 0, &modelDescriptorPool));
	}
	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		uint64_t minUniformAlignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		singleElementSize = ALIGN_UP_POW2(sizeof(glm::mat4)*2, minUniformAlignment);
		createBuffer(context, &modelUniformBuffers[i], singleElementSize*2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	{
		VkDescriptorSetLayoutBinding bindings[] = {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &sampler},
		};
		VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		createInfo.bindingCount = ARRAY_COUNT(bindings);
		createInfo.pBindings = bindings;
		VKA(vkCreateDescriptorSetLayout(context->device, &createInfo, 0, &modelDescriptorSetLayout));

		for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorSetAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
			allocateInfo.descriptorPool = modelDescriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &modelDescriptorSetLayout;
			VKA(vkAllocateDescriptorSets(context->device, &allocateInfo, &modelDescriptorSets[i]));

			VkDescriptorBufferInfo bufferInfo = {modelUniformBuffers[i].buffer, 0, sizeof(glm::mat4)*2};
			VkDescriptorImageInfo imageInfo = {sampler, model.albedoTexture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			VkWriteDescriptorSet descriptorWrites[2];
			descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			descriptorWrites[0].dstSet = modelDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			descriptorWrites[1].dstSet = modelDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].pImageInfo = &imageInfo;
			VK(vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, 0));
		}
	}
	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		VkQueryPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = 64;
		VKA(vkCreateQueryPool(context->device, &createInfo, 0, &timestampQueryPools[i]));
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
									vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding, 1, &spriteDescriptorLayout, 0);


	VkVertexInputAttributeDescription modelAttributeDescriptions[3] = {};
	modelAttributeDescriptions[0].binding = 0;
	modelAttributeDescriptions[0].location = 0;
	modelAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	modelAttributeDescriptions[0].offset = 0;
	modelAttributeDescriptions[1].binding = 0;
	modelAttributeDescriptions[1].location = 1;
	modelAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	modelAttributeDescriptions[1].offset = sizeof(float) * 3;
	modelAttributeDescriptions[2].binding = 0;
	modelAttributeDescriptions[2].location = 2;
	modelAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	modelAttributeDescriptions[2].offset = sizeof(float) * 6;
	VkVertexInputBindingDescription modelInputBinding = {};
	modelInputBinding.binding = 0;
	modelInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	modelInputBinding.stride = sizeof(float) * 8;
	modelPipeline = createPipeline(context, "../shaders/model_vert.spv", "../shaders/model_frag.spv", renderPass, swapchain.width, swapchain.height,
									modelAttributeDescriptions, ARRAY_COUNT(modelAttributeDescriptions), &modelInputBinding, 1, &modelDescriptorSetLayout, 0);

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

	createBuffer(context, &spriteVertexBuffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &spriteVertexBuffer, vertexData, sizeof(vertexData));
	
	createBuffer(context, &spriteIndexBuffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &spriteIndexBuffer, indexData, sizeof(indexData));

	{ // Init camera
		camera.cameraPosition = glm::vec3(0.0f);
		camera.cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
		camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		camera.yaw = 0.0f;
		camera.pitch = 0.0f;
	}

	// Init ImGui
	{
		VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
		VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * ARRAY_COUNT(poolSizes);
        poolInfo.poolSizeCount = (uint32_t)ARRAY_COUNT(poolSizes);
        poolInfo.pPoolSizes = poolSizes;
        VKA(vkCreateDescriptorPool(context->device, &poolInfo, 0, &imguiDescriptorPool));
	}
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForVulkan(window);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = context->instance;
	initInfo.PhysicalDevice = context->physicalDevice;
	initInfo.Device = context->device;
	initInfo.QueueFamily = context->graphicsQueue.familyIndex;
	initInfo.Queue = context->graphicsQueue.queue;
	initInfo.DescriptorPool = imguiDescriptorPool;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = swapchain.images.size();
	initInfo.MSAASamples = VK_SAMPLE_COUNT_4_BIT;
	ImGui_ImplVulkan_Init(&initInfo, renderPass);
	// Upload Fonts
    {
        // Use any command queue
        VkCommandPool commandPool = commandPools[0];
        VkCommandBuffer commandBuffer = commandBuffers[0];

        VKA(vkResetCommandPool(context->device, commandPool, 0));
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VKA(vkBeginCommandBuffer(commandBuffer, &begin_info));

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		VKA(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, VK_NULL_HANDLE));

        VKA(vkDeviceWaitIdle(context->device));
        ImGui_ImplVulkan_DestroyFontUploadObjects();
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
	static double frameGpuAvg = 0.0;
	time += 0.01f;
	greenChannel += 0.01f;
	if (greenChannel > 1.0f) greenChannel = 0.0f;
	uint32_t imageIndex = 0;
	static uint32_t frameIndex = 0;

	// Wait for the n-2 frame to finish to be able to reuse its acquireSemaphore in vkAcquireNextImageKHR
	VKA(vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));

	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		// ImGui still expects us to call render so we still do this here
		ImGui::Render();
		// Swapchain is out of date
		recreateSwapchain();
		return;
	} else {
		VKA(vkResetFences(context->device, 1, &fences[frameIndex]));
		if(result != VK_SUBOPTIMAL_KHR) {
			ASSERT_VULKAN(result);
		}
	}

	// Query timestamps
	uint64_t timestamps[2] = {};
	VkResult timestampsValid = VK(vkGetQueryPoolResults(context->device, timestampQueryPools[frameIndex], 0, ARRAY_COUNT(timestamps), sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT));
	if(timestampsValid == VK_SUCCESS) {
		double frameGpuBegin = double(timestamps[0]) * context->physicalDeviceProperties.limits.timestampPeriod * 1e-6;
		double frameGpuEnd = double(timestamps[1]) * context->physicalDeviceProperties.limits.timestampPeriod * 1e-6;
		frameGpuAvg = frameGpuAvg * 0.95 + (frameGpuEnd - frameGpuBegin) * 0.05;
		//LOG_INFO("GPU frametime: ", frameGpuAvg, "ms");
	}

	VKA(vkResetCommandPool(context->device, commandPools[frameIndex], 0));

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	{
		VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
		VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VK(vkCmdResetQueryPool(commandBuffer, timestampQueryPools[frameIndex], 0, 64));
		VK(vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timestampQueryPools[frameIndex], 0));

		VkViewport viewport = { 0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height, 0.0f, 1.0f};
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkClearValue clearValues[2] = {
			{0.0f, greenChannel, 1.0f, 1.0f},
			{0.0f, 0.0f},
		};
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height} };
		beginInfo.clearValueCount = ARRAY_COUNT(clearValues);
		beginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

#if 0
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline.pipeline);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &spriteVertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, spriteIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline.pipelineLayout, 0, 1, &spriteDescriptorSet, 0, 0);
		vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indexData), 1, 0, 0, 0);
#else
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f));
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -time, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;

		glm::mat4 modelViewProj = camera.viewProj * modelMatrix;
		glm::mat4 modelView = camera.view * modelMatrix;

		void* mapped;
		VK(vkMapMemory(context->device, modelUniformBuffers[frameIndex].memory, 0, sizeof(glm::mat4)*2, 0, &mapped));
		memcpy(mapped, &modelViewProj, sizeof(modelViewProj));
		memcpy(((uint8_t*)mapped)+sizeof(glm::mat4), &modelView, sizeof(modelView));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipeline);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		uint32_t dynamicOffset = 0;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipelineLayout, 0, 1, &modelDescriptorSets[frameIndex], 1, &dynamicOffset);
		vkCmdDrawIndexed(commandBuffer, model.numIndices, 1, 0, 0, 0);


		// Second instance
		modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)) * scaleMatrix * rotationMatrix;
		modelViewProj = camera.viewProj * modelMatrix;
		modelView = camera.view * modelMatrix;

		mapped = ((uint8_t*)mapped) + singleElementSize;
		memcpy(mapped, &modelViewProj, sizeof(modelViewProj));
		memcpy(((uint8_t*)mapped)+sizeof(glm::mat4), &modelView, sizeof(modelView));

		dynamicOffset = singleElementSize;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline.pipelineLayout, 0, 1, &modelDescriptorSets[frameIndex], 1, &dynamicOffset);
		vkCmdDrawIndexed(commandBuffer, model.numIndices, 1, 0, 0, 0);

		VK(vkUnmapMemory(context->device, modelUniformBuffers[frameIndex].memory));
#endif

		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

		vkCmdEndRenderPass(commandBuffer);

		VK(vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, timestampQueryPools[frameIndex], 1));

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

	// Destroy ImGui
	ImGui_ImplVulkan_Shutdown();
	VK(vkDestroyDescriptorPool(context->device, imguiDescriptorPool, 0));
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	VK(vkDestroyDescriptorPool(context->device, modelDescriptorPool, 0));
	VK(vkDestroyDescriptorSetLayout(context->device, modelDescriptorSetLayout, 0));
	destroyModel(context, &model);
	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		destroyBuffer(context, &modelUniformBuffers[i]);
	}

	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		vkDestroyQueryPool(context->device, timestampQueryPools[i], 0);
	}

	VK(vkDestroyDescriptorPool(context->device, spriteDescriptorPool, 0));
	VK(vkDestroyDescriptorSetLayout(context->device, spriteDescriptorLayout, 0));
	destroyBuffer(context, &spriteVertexBuffer);
	destroyBuffer(context, &spriteIndexBuffer);
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
	for(uint32_t i = 0; i < depthBuffers.size(); ++i) {
		destroyImage(context, &depthBuffers[i]);
	}
	for(uint32_t i = 0; i < colorBuffers.size(); ++i) {
		destroyImage(context, &colorBuffers[i]);
	}
	colorBuffers.clear();
	depthBuffers.clear();
	destroyRenderpass(context, renderPass);
	destroySwapchain(context, &swapchain);
	VK(vkDestroySurfaceKHR(context->instance, surface, 0));
	exitVulkan(context);
}

void updateApplication(float delta) {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const uint8_t* keys = SDL_GetKeyboardState(0);
	int mouseX, mouseY;
	uint32_t mouseButtons = SDL_GetRelativeMouseState(&mouseX, &mouseY);

	if(SDL_GetRelativeMouseMode()) {
		float cameraSpeed = 5.0f;
		float mouseSensitivity = 0.27f;

		if(keys[SDL_SCANCODE_W]) {
			camera.cameraPosition += glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)*camera.cameraDirection) * delta * cameraSpeed;
		}
		if(keys[SDL_SCANCODE_S]) {
			camera.cameraPosition -= glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)*camera.cameraDirection) * delta * cameraSpeed;
		}
		if(keys[SDL_SCANCODE_A]) {
			camera.cameraPosition += glm::normalize(glm::cross(camera.cameraDirection, camera.up)) * delta * cameraSpeed;
		}
		if(keys[SDL_SCANCODE_D]) {
			camera.cameraPosition -= glm::normalize(glm::cross(camera.cameraDirection, camera.up)) * delta * cameraSpeed;
		}
		if(keys[SDL_SCANCODE_SPACE]) {
			camera.cameraPosition += glm::normalize(camera.up) * delta * cameraSpeed;
		}
		if(keys[SDL_SCANCODE_LSHIFT]) {
			camera.cameraPosition -= glm::normalize(camera.up) * delta * cameraSpeed;
		}

		camera.yaw += mouseX * mouseSensitivity;
		camera.pitch -= mouseY * mouseSensitivity;
	}

	if(camera.pitch > 89.0f) camera.pitch = 89.0f;
	if(camera.pitch < -89.0f) camera.pitch = -89.0f;
	glm::vec3 front;
	front.x = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));
	front.y = sin(glm::radians(camera.pitch));
	front.z = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
	camera.cameraDirection = glm::normalize(front);
	camera.proj = getProjectionInverseZ(glm::radians(45.0f), swapchain.width, swapchain.height, 0.01f);
	camera.view = glm::lookAtLH(camera.cameraPosition, camera.cameraPosition + camera.cameraDirection, camera.up);
	camera.viewProj = camera.proj * camera.view;

	static bool showDemoWindow = true;
	if(showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}
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

	float delta = 0.0f;
	double frameCpuAvg = 0.0f;
	uint64_t perfCounterFrequency = SDL_GetPerformanceFrequency();
	uint64_t lastCounter = SDL_GetPerformanceCounter();
	while (handleMessage()) {
		updateApplication(delta);
		renderApplication();

		uint64_t endCounter = SDL_GetPerformanceCounter();
		uint64_t counterElapsed = endCounter - lastCounter;
		frameCpuAvg = frameCpuAvg * 0.95f + delta * 0.05f * 1000.0f;
		//LOG_INFO("CPU frametime: ", frameCpuAvg, "ms");
		delta = ((float)counterElapsed) / (float) perfCounterFrequency;
		lastCounter = endCounter;
	}

	shutdownApplication();

	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}