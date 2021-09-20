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
};

struct VulkanContext {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	VulkanQueue graphicsQueue;
};

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions);
void exitVulkan(VulkanContext* context);

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);