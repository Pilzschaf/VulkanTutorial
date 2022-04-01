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
				return i;
			}
		}
	}

	// No matching avaialble memroy type found
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

void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer) {
	VK(vkDestroyBuffer(context->device, buffer->buffer, 0));
	// Assumes that the buffer owns its own memory block
	VK(vkFreeMemory(context->device, buffer->memory, 0));
}