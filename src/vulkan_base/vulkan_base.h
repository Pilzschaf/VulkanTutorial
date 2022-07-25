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

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions);
void exitVulkan(VulkanContext* context);

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain = 0);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);

void createBuffer(VulkanContext* context, VulkanBuffer* buffer, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
void uploadDataToBuffer(VulkanContext* context, VulkanBuffer* buffer, void* data, size_t size);
void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer);

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height, VkVertexInputAttributeDescription* attributes, uint32_t numAttributes, VkVertexInputBindingDescription* binding);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);
