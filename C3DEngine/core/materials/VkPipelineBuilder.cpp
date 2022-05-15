
#include "VkPipelineBuilder.h"
#include "../VkInitializers.h"
#include "../Logger.h"
#include "../shaders/ShaderEffect.h"

namespace C3D
{
	VkPipeline VkPipelineBuilder::Build(VkDevice device, VkRenderPass pass)
	{
		vertexInputInfo = VkInit::VertexInputStateCreateInfo();

		vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());

		vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());

		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.pNext = nullptr;

		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());

		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multiSampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencil;

		pipelineInfo.layout = layout;
		pipelineInfo.renderPass = pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		std::vector<VkDynamicState> dynamicStates;
		dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

		VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicState.pDynamicStates = dynamicStates.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

		pipelineInfo.pDynamicState = &dynamicState;

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		{
			Logger::Error("Failed to build Graphics Pipeline");
			return VK_NULL_HANDLE;
		}
		return pipeline;
	}

	void VkPipelineBuilder::ClearVertexInput()
	{
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
	}

	void VkPipelineBuilder::SetShaders(const ShaderEffect* effect)
	{
		shaderStages.clear();
		effect->FillStages(shaderStages);
		layout = effect->builtLayout;
	}

	VkPipeline VkComputePipelineBuilder::Build(VkDevice device) const
	{
		VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		pipelineInfo.pNext = nullptr;

		pipelineInfo.stage = shaderStage;
		pipelineInfo.layout = layout;

		VkPipeline pipeline;
		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		{
			Logger::Error("Failed to build Compute Pipeline");
			return VK_NULL_HANDLE;
		}
		return pipeline;
	}
}
