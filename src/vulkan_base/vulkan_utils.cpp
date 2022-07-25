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