#include "vulkan_base.h"

VkBool32 VKAPI_CALL debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		LOG_ERROR(callbackData->pMessage);
	} else {
		LOG_WARN(callbackData->pMessage);
	}
	return VK_FALSE;
}

VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance) {
	PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebutUtilsMessengerEXT;
	pfnCreateDebutUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	VkDebugUtilsMessengerCreateInfoEXT callbackInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	callbackInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	callbackInfo.pfnUserCallback = debugReportCallback;

	VkDebugUtilsMessengerEXT callback = 0;
	VKA(pfnCreateDebutUtilsMessengerEXT(instance, &callbackInfo, 0, &callback));

	return callback;
}

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

	VkValidationFeatureEnableEXT enableValidationFeatures[] = {
		//VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	};
	VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = ARRAY_COUNT(enableValidationFeatures);
	validationFeatures.pEnabledValidationFeatures = enableValidationFeatures;

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
	applicationInfo.apiVersion = VK_API_VERSION_1_0;
	
	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pNext = &validationFeatures;
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledLayerCount = ARRAY_COUNT(enabledLayers);
	createInfo.ppEnabledLayerNames = enabledLayers;
	createInfo.enabledExtensionCount = instanceExtensionCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions;

	if (VK(vkCreateInstance(&createInfo, 0, &context->instance)) != VK_SUCCESS) {
		LOG_ERROR("Error creating vulkan instance");
		return false;
	}

	context->debugCallback = registerDebugCallback(context->instance);

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

bool createLogicalDevice(VulkanContext* context, uint32_t deviceExtensionCount, const char** deviceExtensions) {
	// Queues
	uint32_t numQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, 0);
	VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[numQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies);

	uint32_t graphicsQueueIndex = 0;
	for (uint32_t i = 0; i < numQueueFamilies; ++i) {
		VkQueueFamilyProperties queueFamily = queueFamilies[i];
		if (queueFamily.queueCount > 0) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsQueueIndex = i;
				break;
			}
		}
	}

	float priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = priorities;

	VkPhysicalDeviceFeatures enabledFeatures = {};

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;
	createInfo.pEnabledFeatures = &enabledFeatures;

	if (vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device)) {
		LOG_ERROR("Failed to create vulkan logical device");
		return false;
	}

	// Acquire queues
	context->graphicsQueue.familyIndex = graphicsQueueIndex;
	VK(vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue));

	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VK(vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties));
	LOG_INFO("Num device memory heaps: ", deviceMemoryProperties.memoryHeapCount);
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryHeapCount; ++i) {
		const char* isDeviceLocal = "false";
		if (deviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			isDeviceLocal = "true";
		}
		LOG_INFO("Heap ", i, ": Size:", deviceMemoryProperties.memoryHeaps[i].size, "bytes Device local: ", isDeviceLocal);
	}

	return true;
}

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions) {
	VulkanContext* context = new VulkanContext;

	if (!initVulkanInstance(context, instanceExtensionCount, instanceExtensions)) {
		return 0;
	}

	if (!selectPhysicalDevice(context)) {
		return 0;
	}

	if (!createLogicalDevice(context, deviceExtensionCount, deviceExtensions)) {
		return 0;
	}

	return context;
}

void exitVulkan(VulkanContext* context) {
	VKA(vkDeviceWaitIdle(context->device));
	VK(vkDestroyDevice(context->device, 0));

	if (context->debugCallback) {
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
		pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
		pfnDestroyDebugUtilsMessengerEXT(context->instance, context->debugCallback, 0);
		context->debugCallback = 0;
	}
	vkDestroyInstance(context->instance, 0);
}