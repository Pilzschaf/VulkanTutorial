#include "vulkan_base.h"

bool initVulkanInstance(VulkanContext* context) {
	VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	applicationInfo.pApplicationName = "Vulkan Tutorial";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1); // 0.0.1
	applicationInfo.apiVersion = VK_VERSION_1_2;
	
	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &applicationInfo;

	if (vkCreateInstance(&createInfo, 0, &context->instance) != VK_SUCCESS) {
		LOG_ERROR("Error creating vulkan instance");
		return false;
	}

	return true;
}

VulkanContext* initVulkan() {
	VulkanContext* context = new VulkanContext;

	if (!initVulkanInstance(context)) {
		return 0;
	}

	return context;
}