#include "vulkan_base.h"

uint32_t findMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) {
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VK(vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties));

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i) {
		// Check if required memory type is allowed
		if ((typeFilter & (1 << i)) != 0) {
			// Check if required properties are satisfied
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
				// Return this memory type index
				LOG_INFO("Using memory heap index ", deviceMemoryProperties.memoryTypes[i].heapIndex);
				return i;
			}
		}
	}

	// No matching avaialble memory type found
	assert(false);
	return UINT32_MAX;
}

void createBuffer(VulkanContext* context, VulkanBuffer* buffer, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = usage;
	VKA(vkCreateBuffer(context->device, &createInfo, 0, &buffer->buffer));

	VkMemoryRequirements memoryRequirements;
	VK(vkGetBufferMemoryRequirements(context->device, buffer->buffer, &memoryRequirements));

	uint32_t memoryIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, memoryProperties);
	assert(memoryIndex != UINT32_MAX);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryIndex;

	VKA(vkAllocateMemory(context->device, &allocateInfo, 0, &buffer->memory));

	VKA(vkBindBufferMemory(context->device, buffer->buffer, buffer->memory, 0));
}

void uploadDataToBuffer(VulkanContext* context, VulkanBuffer* buffer, void* data, size_t size) {
#if 0
	void* mapped;
	VKA(vkMapMemory(context->device, buffer->memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	VK(vkUnmapMemory(context->device, buffer->memory));
#else
	// Upload with staging buffer
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VulkanBuffer stagingBuffer;
	createBuffer(context, &stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mapped;
	VKA(vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	VK(vkUnmapMemory(context->device, stagingBuffer.memory));
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = queue->familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPool));
	}
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer));
	}

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkBufferCopy region = { 0, 0, size };
	VK(vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer->buffer, 1, &region));

	VKA(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VKA(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
	VKA(vkQueueWaitIdle(queue->queue));

	VK(vkDestroyCommandPool(context->device, commandPool, 0));
	destroyBuffer(context, &stagingBuffer);
#endif
}

void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer) {
	VK(vkDestroyBuffer(context->device, buffer->buffer, 0));
	// Assumes that the buffer owns its own memory block
	VK(vkFreeMemory(context->device, buffer->memory, 0));
}

void createImage(VulkanContext* context, VulkanImage* image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
	{
		VkImageCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = width;
		createInfo.extent.height = height;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.format = format;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.usage = usage;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VKA(vkCreateImage(context->device, &createInfo, 0, &image->image));
	}

	VkMemoryRequirements memoryRequirements;
	VK(vkGetImageMemoryRequirements(context->device, image->image, &memoryRequirements));
	VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VKA(vkAllocateMemory(context->device, &allocateInfo, 0, &image->memory));
	VKA(vkBindImageMemory(context->device, image->image, image->memory, 0));

	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if(format == VK_FORMAT_D32_SFLOAT) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	{
		VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		createInfo.image = image->image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = aspect;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.layerCount = 1;
		VKA(vkCreateImageView(context->device, &createInfo, 0, &image->view));
	}
}

void uploadDataToImage(VulkanContext* context, VulkanImage* image, void* data, size_t size, uint32_t width, uint32_t height, VkImageLayout finalLayout, VkAccessFlags dstAccessMask) {
	// Upload with staging buffer
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VulkanBuffer stagingBuffer;
	createBuffer(context, &stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mapped;
	VKA(vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	VK(vkUnmapMemory(context->device, stagingBuffer.memory));
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = queue->familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPool));
	}
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer));
	}

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	{
		VkImageMemoryBarrier imageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = image->image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {width, height, 1};
	VK(vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

	{
		VkImageMemoryBarrier imageBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = finalLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = image->image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_NONE;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}

	VKA(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VKA(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
	VKA(vkQueueWaitIdle(queue->queue));

	VK(vkDestroyCommandPool(context->device, commandPool, 0));
	destroyBuffer(context, &stagingBuffer);
}

void destroyImage(VulkanContext* context, VulkanImage* image) {
	VK(vkDestroyImageView(context->device, image->view, 0));
	VK(vkDestroyImage(context->device, image->image, 0));
	VK(vkFreeMemory(context->device, image->memory, 0));
}