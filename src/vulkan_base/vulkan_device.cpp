#include "vulkan_base.h"

bool initVulkanInstance(VulkanContext* context, uint32_t instanceExtensionCount, const char** instanceExtensions) {
	uint32_t layerPropertyCount = 0;
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0));
	VkLayerProperties* layerProperties = new VkLayerProperties[layerPropertyCount];
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties));
	for (uint32_t i = 0; i < layerPropertyCount; ++i) {
#ifdef VULKAN_INFO_OUTPUT
		LOG_INFO(layerProperties[i].layerName);
		LOG_INFO(layerProperties[i].description);
#endif
	}
	delete[] layerProperties;

	const char* enabledLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

#ifdef VULKAN_INFO_OUTPUT
	uint32_t availableInstanceExtensionCount;
	VKA(vkEnumerateInstanceExtensionProperties(0, &availableInstanceExtensionCount, 0));
	VkExtensionProperties* instanceExtensionProperties = new VkExtensionProperties[availableInstanceExtensionCount];
	VKA(vkEnumerateInstanceExtensionProperties(0, &availableInstanceExtensionCount, instanceExtensionProperties));
	for (uint32_t i = 0; i < availableInstanceExtensionCount; ++i) {
		LOG_INFO(instanceExtensionProperties[i].extensionName);
	}
	delete[] instanceExtensionProperties;
#endif
	
	VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	applicationInfo.pApplicationName = "Vulkan Tutorial";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1); // 0.0.1
	applicationInfo.apiVersion = VK_API_VERSION_1_2;
	
	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledLayerCount = ARRAY_COUNT(enabledLayers);
	createInfo.ppEnabledLayerNames = enabledLayers;
	createInfo.enabledExtensionCount = instanceExtensionCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions;

	if (VK(vkCreateInstance(&createInfo, 0, &context->instance)) != VK_SUCCESS) {
		LOG_ERROR("Error creating vulkan instance");
		return false;
	}

	return true;
}

bool selectPhysicalDevice(VulkanContext* context) {
	uint32_t numDevices = 0;
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, 0));
	if (numDevices == 0) {
		LOG_ERROR("Failed to find GPUs with Vulkan support!");
		context->physicalDevice = 0;
		return false;
	}
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numDevices];
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices));
	LOG_INFO("Found ", numDevices, " GPU(s):");
	for(uint32_t i = 0; i < numDevices; ++i) {
		VkPhysicalDeviceProperties properties = {};
		VK(vkGetPhysicalDeviceProperties(physicalDevices[i], &properties));
		LOG_INFO("GPU", i, ": ", properties.deviceName);
	}

	context->physicalDevice = physicalDevices[0];
	VK(vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties));
	LOG_INFO("Selected GPU: ", context->physicalDeviceProperties.deviceName);

	delete[] physicalDevices;

	return true;
}

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions) {
	VulkanContext* context = new VulkanContext;

	if (!initVulkanInstance(context, instanceExtensionCount, instanceExtensions)) {
		return 0;
	}

	if (!selectPhysicalDevice(context)) {
		return 0;
	}

	return context;
}