#include "vulkan_base.h"

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format) {
	VkRenderPass renderPass;

	VkAttachmentDescription attachmentDescriptions[2];
	attachmentDescriptions[0] = {};
	attachmentDescriptions[0].format = format;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescriptions[1] = {};
	attachmentDescriptions[1].format = VK_FORMAT_D32_SFLOAT;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthStencilReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	subpass.pDepthStencilAttachment = &depthStencilReference;

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