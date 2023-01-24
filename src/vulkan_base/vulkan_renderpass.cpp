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
	VkAttachmentReference postprocessTargetReference = {2, VK_IMAGE_LAYOUT_GENERAL};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	subpass.pDepthStencilAttachment = &depthStencilReference;
	subpass.pResolveAttachments = &resolveTargetReference;

	VkSubpassDescription postProcessSubpass = {};
	postProcessSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	postProcessSubpass.colorAttachmentCount = 1;
	postProcessSubpass.pColorAttachments = &postprocessTargetReference;
	postProcessSubpass.inputAttachmentCount = 1;
	postProcessSubpass.pInputAttachments = &postprocessTargetReference;

	VkSubpassDescription subpasses[] = {subpass, postProcessSubpass};
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dstSubpass = 1;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = ARRAY_COUNT(attachmentDescriptions);
	createInfo.pAttachments = attachmentDescriptions;
	createInfo.subpassCount = ARRAY_COUNT(subpasses);
	createInfo.pSubpasses = subpasses;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &subpassDependency;
	VKA(vkCreateRenderPass(context->device, &createInfo, 0, &renderPass));

	return renderPass;
}

void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass) {
	VK(vkDestroyRenderPass(context->device, renderPass, 0));
}