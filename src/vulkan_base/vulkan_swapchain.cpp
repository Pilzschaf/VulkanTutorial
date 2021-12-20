#include "vulkan_base.h"

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain) {
	VulkanSwapchain result = {};

	VkBool32 supportsPresent = 0;
	VKA(vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, surface, &supportsPresent));
	if (!supportsPresent) {
		LOG_ERROR("Graphics queue does not support present");
		return result;
	}

	uint32_t numFormats = 0;
	VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, 0));
	VkSurfaceFormatKHR* availableFormats = new VkSurfaceFormatKHR[numFormats];
	VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats));
	if (numFormats <= 0) {
		LOG_ERROR("No surface formats available");
		delete[] availableFormats;
		return result;
	}

	// First available format should be a sensible default in most cases
	VkFormat format = availableFormats[0].format;
	VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
		surfaceCapabilities.currentExtent.width = surfaceCapabilities.minImageExtent.width;
	}
	if (surfaceCapabilities.currentExtent.height == 0xFFFFFFFF) {
		surfaceCapabilities.currentExtent.height = surfaceCapabilities.minImageExtent.height;
	}
	if (surfaceCapabilities.maxImageCount == 0) {
		surfaceCapabilities.maxImageCount = 8;
	}

	VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = surface;
	createInfo.minImageCount = 3;
	createInfo.imageFormat = format;
	createInfo.imageColorSpace = colorSpace;
	createInfo.imageExtent = surfaceCapabilities.currentExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = usage;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.oldSwapchain = oldSwapchain ? oldSwapchain->swapchain : 0;
	VKA(vkCreateSwapchainKHR(context->device, &createInfo, 0, &result.swapchain));

	result.format = format;
	result.width = surfaceCapabilities.currentExtent.width;
	result.height = surfaceCapabilities.currentExtent.height;

	uint32_t numImages;
	VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, 0));

	// Acquire swapchain images
	result.images.resize(numImages);
	VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, result.images.data()));

	// Create image views
	result.imageViews.resize(numImages);
	for (uint32_t i = 0; i < numImages; ++i) {
		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.image = result.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components = {};
		createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VKA(vkCreateImageView(context->device, &createInfo, 0, &result.imageViews[i]));
	}

	delete[] availableFormats;
	return result;
}

void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain) {
	for (uint32_t i = 0; i < swapchain->imageViews.size(); ++i) {
		VK(vkDestroyImageView(context->device, swapchain->imageViews[i], 0));
	}
	VK(vkDestroySwapchainKHR(context->device, swapchain->swapchain, 0));
}