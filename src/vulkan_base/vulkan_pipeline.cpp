#include "vulkan_base.h"

VkShaderModule createShaderModule(VulkanContext* context, const char* shaderFilename) {
	VkShaderModule result = {};

	// Read shader file
	FILE* file = fopen(shaderFilename, "rb");
	if (!file) {
		LOG_ERROR("Shader not found: ", shaderFilename);
		return result;
	}
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	assert((fileSize & 0x03) == 0);
	uint8_t* buffer = new uint8_t[fileSize];
	fread(buffer, 1, fileSize, file);

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = fileSize;
	createInfo.pCode = (uint32_t*)buffer;
	VKA(vkCreateShaderModule(context->device, &createInfo, 0, &result));

	delete[] buffer;
	fclose(file);

	return result;
}

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height,
							  VkVertexInputAttributeDescription* attributes, uint32_t numAttributes, VkVertexInputBindingDescription* binding, uint32_t numSetLayouts, VkDescriptorSetLayout* setLayouts, VkPushConstantRange* pushConstant, uint32_t subpassIndex, VkSampleCountFlagBits sampleCount, VkSpecializationInfo* specializationInfo, VkPipelineCache pipelineCache) {
	VkShaderModule vertexShaderModule = createShaderModule(context, vertexShaderFilename);
	VkShaderModule fragmentShaderModule = createShaderModule(context, fragmentShaderFilename);

	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = specializationInfo;
	shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShaderModule;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = specializationInfo;

	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputState.vertexBindingDescriptionCount = binding ? 1 : 0;
	vertexInputState.pVertexBindingDescriptions = binding;
	vertexInputState.vertexAttributeDescriptionCount = numAttributes;
	vertexInputState.pVertexAttributeDescriptions = attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	//VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height };
	//viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	//VkRect2D scissor = { {0, 0}, {width, height} };
	//viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = sampleCount;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;

	VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	dynamicState.dynamicStateCount = ARRAY_COUNT(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayout pipelineLayout;
	{
		VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = numSetLayouts;
		createInfo.pSetLayouts = setLayouts;
		createInfo.pushConstantRangeCount = pushConstant ? 1 : 0;
		createInfo.pPushConstantRanges = pushConstant;
		VKA(vkCreatePipelineLayout(context->device, &createInfo, 0, &pipelineLayout));
	}

	VkPipeline pipeline;
	{
		VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		createInfo.stageCount = ARRAY_COUNT(shaderStages);
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputState;
		createInfo.pInputAssemblyState = &inputAssemblyState;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizationState;
		createInfo.pMultisampleState = &multisampleState;
		createInfo.pDepthStencilState = &depthStencilState;
		createInfo.pColorBlendState = &colorBlendState;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = pipelineLayout;
		createInfo.renderPass = renderPass;
		createInfo.subpass = subpassIndex;
		VKA(vkCreateGraphicsPipelines(context->device, pipelineCache, 1, &createInfo, 0, &pipeline));
	}

	// Module can be destroyed after pipeline creation
	VK(vkDestroyShaderModule(context->device, vertexShaderModule, 0));
	VK(vkDestroyShaderModule(context->device, fragmentShaderModule, 0));

	VulkanPipeline result = {};
	result.pipeline = pipeline;
	result.pipelineLayout = pipelineLayout;
	return result;
}

VulkanPipeline createComputePipeline(VulkanContext* context, const char* shaderFilename,
							  		 uint32_t numSetLayouts, VkDescriptorSetLayout* setLayouts, VkPushConstantRange* pushConstant, VkSpecializationInfo* specializationInfo, VkPipelineCache pipelineCache) {
	VkShaderModule shaderModule = createShaderModule(context, shaderFilename);
	VkPipelineShaderStageCreateInfo shaderStage;
	shaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = shaderModule;
	shaderStage.pName = "main";
	shaderStage.pSpecializationInfo = specializationInfo;

	VkPipelineLayout pipelineLayout;
	{
		VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = numSetLayouts;
		createInfo.pSetLayouts = setLayouts;
		createInfo.pushConstantRangeCount = pushConstant ? 1 : 0;
		createInfo.pPushConstantRanges = pushConstant;
		VKA(vkCreatePipelineLayout(context->device, &createInfo, 0, &pipelineLayout));
	}

	VkPipeline pipeline;
	{
		VkComputePipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
		createInfo.stage = shaderStage;
		createInfo.layout = pipelineLayout;
		VKA(vkCreateComputePipelines(context->device, pipelineCache, 1, &createInfo, 0, &pipeline));
	}

	// Module can be destroyed after pipeline creation
	VK(vkDestroyShaderModule(context->device, shaderModule, 0));

	VulkanPipeline result = {};
	result.pipeline = pipeline;
	result.pipelineLayout = pipelineLayout;
	return result;
}

void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline) {
	VK(vkDestroyPipeline(context->device, pipeline->pipeline, 0));
	VK(vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, 0));
}

VkPipelineCache createPipelineCache(VulkanContext* context, const char* filename) {
	VkPipelineCache result = {0};

	FILE* file = fopen(filename, "rb");
	long fileSize = 0;
	uint8_t* buffer = 0;
	if(file) {
		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		buffer = new uint8_t[fileSize];
		fread(buffer, 1, fileSize, file);
	}

	VkPipelineCacheCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	if(buffer) {
		createInfo.initialDataSize = fileSize;
		createInfo.pInitialData = buffer;
	}
	vkCreatePipelineCache(context->device, &createInfo, 0, &result);

	if(buffer) {
		delete[] buffer;
	}
	return result;
}

void destroyPipelineCache(VulkanContext* context, VkPipelineCache cache, const char* filename) {
	size_t cacheSize = 0;
	vkGetPipelineCacheData(context->device, cache, &cacheSize, 0);
	uint8_t* cacheData = new uint8_t[cacheSize];
	vkGetPipelineCacheData(context->device, cache, &cacheSize, cacheData);
	FILE* file = fopen(filename, "wb");
	fwrite(cacheData, sizeof(uint8_t), cacheSize, file);
	delete[] cacheData;
	vkDestroyPipelineCache(context->device, cache, 0);
}