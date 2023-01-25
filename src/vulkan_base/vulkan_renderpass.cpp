#include "vulkan_base.h"

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format, VkSampleCountFlagBits sampleCount, bool useDepth, VkImageLayout finalLayout) {
	VkRenderPass renderPass;

	VkAttachmentDescription attachmentDescriptions[3];
	attachmentDescriptions[0] = {};
	attachmentDescriptions[0].format = format;
	attachmentDescriptions[0].samples = sampleCount;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = finalLayout;
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
	attachmentDescriptions[2].finalLayout = finalLayout;

	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthStencilReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	VkAttachmentReference resolveTargetReference = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	uint32_t usedAttachmentCount = 1;
	if(useDepth) {
		usedAttachmentCount++;
		subpass.pDepthStencilAttachment = &depthStencilReference;
	}
	if(sampleCount != VK_SAMPLE_COUNT_1_BIT) {
		if(!useDepth) {
			attachmentDescriptions[1] = attachmentDescriptions[2];
			resolveTargetReference.attachment = 1;
		}
		usedAttachmentCount++;
		subpass.pResolveAttachments = &resolveTargetReference;
	}

	VkSubpassDescription subpasses[] = {subpass};
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = usedAttachmentCount;
	createInfo.pAttachments = attachmentDescriptions;
	createInfo.subpassCount = ARRAY_COUNT(subpasses);
	createInfo.pSubpasses = subpasses;
	if(finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &subpassDependency;
	}
	VKA(vkCreateRenderPass(context->device, &createInfo, 0, &renderPass));

	return renderPass;
}

void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass) {
	VK(vkDestroyRenderPass(context->device, renderPass, 0));
}