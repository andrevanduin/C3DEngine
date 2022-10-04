
#include "vulkan_pipeline.h"
#include "vulkan_utils.h"

#include "core/memory.h"
#include "core/logger.h"

#include "renderer/vertex.h"
#include "services/services.h"

namespace C3D
{
	VulkanPipeline::VulkanPipeline() : layout(nullptr), m_handle(nullptr) {}

	VkCullModeFlagBits GetVkCullMode(const FaceCullMode cullMode)
	{
		switch (cullMode)
		{
			case FaceCullMode::None:
				return VK_CULL_MODE_NONE;
			case FaceCullMode::Front:
				return VK_CULL_MODE_FRONT_BIT;
			case FaceCullMode::Back:
				return VK_CULL_MODE_BACK_BIT;
			case FaceCullMode::FrontAndBack:
				return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_BACK_BIT;
	}

	bool VulkanPipeline::Create(const VulkanContext* context, const VulkanRenderPass* renderPass, u32 stride, u32 attributeCount, VkVertexInputAttributeDescription* attributes,
	                            u32 descriptorSetLayoutCount, VkDescriptorSetLayout* descriptorSetLayouts, u32 stageCount, VkPipelineShaderStageCreateInfo* stages,
	                            VkViewport viewport, VkRect2D scissor, FaceCullMode cullMode, bool isWireFrame, bool depthTestEnabled, u32 pushConstantRangeCount, Range* pushConstantRanges)
	{
		VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = isWireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = GetVkCullMode(cullMode);
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizerCreateInfo.depthBiasClamp = 0.0f;
		rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

		// MultiSampling
		VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multiSampleCreateInfo.minSampleShading = 1.0f;
		multiSampleCreateInfo.pSampleMask = nullptr;
		multiSampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multiSampleCreateInfo.alphaToOneEnable = VK_FALSE;

		// Depth and stencil testing
		VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		if (depthTestEnabled)
		{
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;
		}

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
		Memory.Zero(&colorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));

		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;


		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

		// Dynamic state
		constexpr u32 dynamicStateCount = 3;
		VkDynamicState dynamicStates[dynamicStateCount] = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Vertex Input
		VkVertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0;
		bindingDescription.stride = stride;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Attributes
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = attributeCount;
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributes;

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Pipeline layout create info
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

		// Push constants
		if (pushConstantRangeCount > 0)
		{
			// Note: 32 is the max push constant ranges we can ever have because the Vulkan spec only guarantees 128 bytes with 4-byte alignment.
			if (pushConstantRangeCount > 32)
			{
				Logger::Error("[VULKAN_PIPELINE] - Create() Cannot have more than 32 push constant ranges. Passed count: {}", pushConstantRangeCount);
				return false;
			}

			VkPushConstantRange ranges[32];
			Memory.Zero(ranges, sizeof(VkPushConstantRange) * 32);
			for (u32 i = 0; i < pushConstantRangeCount; i++)
			{
				ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				ranges[i].offset = static_cast<u32>(pushConstantRanges[i].offset);
				ranges[i].size = static_cast<u32>(pushConstantRanges[i].size);
			}

			pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
			pipelineLayoutCreateInfo.pPushConstantRanges = ranges;
		}
		else
		{
			pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		}

		// Descriptor set layouts
		pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;

		// Create our pipeline layout
		VK_CHECK(vkCreatePipelineLayout(context->device.logicalDevice, &pipelineLayoutCreateInfo, context->allocator, &layout));

		// Pipeline create info
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.stageCount = stageCount;
		pipelineCreateInfo.pStages = stages;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssembly;

		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multiSampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = depthTestEnabled ? &depthStencil : nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineCreateInfo.pTessellationState = nullptr;

		pipelineCreateInfo.layout = layout;

		pipelineCreateInfo.renderPass = renderPass->handle;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		// Create our pipeline
		VkResult result = vkCreateGraphicsPipelines(context->device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, context->allocator, &m_handle);
		if (VulkanUtils::IsSuccess(result))
		{
			Logger::Debug("[VULKAN_PIPELINE] - Graphics pipeline created");
			return true;
		}

		Logger::Error("[VULKAN_PIPELINE] - vkCreateGraphicsPipelines failed with: {}", VulkanUtils::ResultString(result, true));
		return false;
	}

	void VulkanPipeline::Destroy(const VulkanContext* context)
	{
		if (m_handle)
		{
			vkDestroyPipeline(context->device.logicalDevice, m_handle, context->allocator);
			m_handle = nullptr;
		}
		if (layout)
		{
			vkDestroyPipelineLayout(context->device.logicalDevice, layout, context->allocator);
			layout = nullptr;
		}
	}

	void VulkanPipeline::Bind(const VulkanCommandBuffer* commandBuffer, const VkPipelineBindPoint bindPoint) const
	{
		vkCmdBindPipeline(commandBuffer->handle, bindPoint, m_handle);
	}
}
