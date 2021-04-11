#include "../logger.h"

#include <vulkan/vulkan.h>

struct VulkanContext {
	VkInstance instance;
};

VulkanContext* initVulkan();