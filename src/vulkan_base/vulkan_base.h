#include "../logger.h"

#include <vulkan/vulkan.h>
#include <cassert>

#define ASSERT_VULKAN(val) if(val != VK_SUCCESS) {assert(false);}
#ifndef VK
#define VK(f) (f)
#endif
#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

struct VulkanContext {
	VkInstance instance;
};

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions);