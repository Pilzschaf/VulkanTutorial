#include "vulkan_base.h"

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format, VkSampleCountFlagBits sampleCount) {
	VkRenderPass renderPass;

	VkAttachmentDescription attachmentDescriptions[3];
	attachmentDescriptions[0] = {};
	attachmentDescriptions[0].format = format;
	attachmentDescriptions[0].samples = sampleCount;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentDescriptions[1] = {};
	attachmentDescriptions[1].format = VK_FORMAT_D32_SFLOAT;
	attachmentDescriptions[1].samples = sampleCount;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescriptions[2] = {};
	attachmentDescriptions[2].format = format;
	attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthStencilReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	VkAttachmentReference resolveTargetReference = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	subpass.pDepthStencilAttachment = &depthStencilReference;
	subpass.pResolveAttachments = &resolveTargetReference;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = ARRAY_COUNT(attachmentDescriptions);
	createInfo.pAttachments = attachmentDescriptions;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	VKA(vkCreateRenderPass(context->device, &createInfo, 0, &renderPass));

	return renderPass;
}

void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass) {
	VK(vkDestroyRenderPass(context->device, renderPass, 0));
}