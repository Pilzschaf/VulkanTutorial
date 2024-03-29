#pragma once

#include "../logger.h"

#include <vulkan/vulkan.h>
#include <cassert>
#include <vector>

#define ASSERT_VULKAN(val) if(val != VK_SUCCESS) {assert(false);}
#ifndef VK
#define VK(f) (f)
#endif
#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define ALIGN_UP_POW2(x, p) (((x)+(p) - 1) &~((p) - 1))

struct VulkanQueue {
	VkQueue queue;
	uint32_t familyIndex;
};

struct VulkanSwapchain {
	VkSwapchainKHR swapchain;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
};

struct VulkanPipeline {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct VulkanContext {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	VulkanQueue graphicsQueue;
	VkDebugUtilsMessengerEXT debugCallback;
};

struct VulkanBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct VulkanImage {
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
};

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions);
void exitVulkan(VulkanContext* context);

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain = 0);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format, VkSampleCountFlagBits sampleCount, bool useDepth, VkImageLayout finalLayout);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);

void createBuffer(VulkanContext* context, VulkanBuffer* buffer, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
void uploadDataToBuffer(VulkanContext* context, VulkanBuffer* buffer, void* data, size_t size);
void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer);

void createImage(VulkanContext* context, VulkanImage* image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
void uploadDataToImage(VulkanContext* context, VulkanImage* image, void* data, size_t size, uint32_t width, uint32_t height, VkImageLayout finalLayout, VkAccessFlags dstAccessMask);
void destroyImage(VulkanContext* context, VulkanImage* image);

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height,
							  VkVertexInputAttributeDescription* attributes, uint32_t numAttributes, VkVertexInputBindingDescription* binding, uint32_t numSetLayouts, VkDescriptorSetLayout* setLayouts, VkPushConstantRange* pushConstant, uint32_t subpassIndex = 0, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkSpecializationInfo* specializationInfo = 0, VkPipelineCache pipelineCache = 0);
VulkanPipeline createComputePipeline(VulkanContext* context, const char* shaderFilename,
							  		 uint32_t numSetLayouts, VkDescriptorSetLayout* setLayouts, VkPushConstantRange* pushConstant, VkSpecializationInfo* specializationInfo, VkPipelineCache pipelineCache = 0);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);
VkPipelineCache createPipelineCache(VulkanContext* context, const char* filename);
void destroyPipelineCache(VulkanContext* context, VkPipelineCache cache, const char* filename);