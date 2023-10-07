
#include "vulkan_pipeline.h"

#include <core/logger.h>
#include <resources/shaders/shader.h>

#include "vulkan_utils.h"

namespace C3D
{
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

    VulkanPipeline::VulkanPipeline(u32 supportedTopologyTypes) : m_supportedTopologyTypes(supportedTopologyTypes) {}

    bool VulkanPipeline::Create(const VulkanContext* context, const VulkanPipelineConfig& config)
    {
        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount                     = 1;
        viewportState.pViewports                        = &config.viewport;
        viewportState.scissorCount                      = 1;
        viewportState.pScissors                         = &config.scissor;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
        };
        rasterizerCreateInfo.depthClampEnable        = VK_FALSE;
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizerCreateInfo.polygonMode             = config.isWireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        rasterizerCreateInfo.lineWidth               = 1.0f;
        rasterizerCreateInfo.cullMode                = GetVkCullMode(config.cullMode);
        rasterizerCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizerCreateInfo.depthBiasEnable         = VK_FALSE;
        rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizerCreateInfo.depthBiasClamp          = 0.0f;
        rasterizerCreateInfo.depthBiasSlopeFactor    = 0.0f;

        // MultiSampling
        VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
        };
        multiSampleCreateInfo.sampleShadingEnable   = VK_FALSE;
        multiSampleCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        multiSampleCreateInfo.minSampleShading      = 1.0f;
        multiSampleCreateInfo.pSampleMask           = nullptr;
        multiSampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multiSampleCreateInfo.alphaToOneEnable      = VK_FALSE;

        // Depth and stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil = {
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
        };
        if (config.shaderFlags & ShaderFlagDepthTest)
        {
            depthStencil.depthTestEnable = VK_TRUE;
            if (config.shaderFlags & ShaderFlagDepthWrite)
            {
                depthStencil.depthWriteEnable = VK_TRUE;
            }
            depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable     = VK_FALSE;
        }

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};

        colorBlendAttachmentState.blendEnable         = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
        };
        colorBlendStateCreateInfo.logicOpEnable   = VK_FALSE;
        colorBlendStateCreateInfo.logicOp         = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments    = &colorBlendAttachmentState;

        // Dynamic state
        constexpr u32 dynamicStateCount                 = 3;
        VkDynamicState dynamicStates[dynamicStateCount] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                            VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
        };
        dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
        dynamicStateCreateInfo.pDynamicStates    = dynamicStates;

        // Vertex Input
        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding   = 0;
        bindingDescription.stride    = config.stride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Attributes
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };
        vertexInputCreateInfo.vertexBindingDescriptionCount   = 1;
        vertexInputCreateInfo.pVertexBindingDescriptions      = &bindingDescription;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = config.attributeCount;
        vertexInputCreateInfo.pVertexAttributeDescriptions    = config.attributes;

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        };
        for (u32 i = 1; i < PRIMITIVE_TOPOLOGY_TYPE_MAX; i = i << 1)
        {
            if (m_supportedTopologyTypes & i)
            {
                const auto ptt = static_cast<PrimitiveTopologyType>(i);
                switch (ptt)
                {
                    case PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                        break;
                    case PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                        break;
                    case PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                        break;
                    case PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                        break;
                    case PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                        break;
                    case PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN:
                        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                        m_currentTopology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                        break;
                    default:
                        Logger::Warn("[VULKAN_PIPELINE] - Create() - Unsupported primitive topology: '{}'. Skipping",
                                     ToUnderlying(ptt));
                }
                break;
            }
        }

        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Pipeline layout create info
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

        // Push constants
        if (config.pushConstantRangeCount > 0)
        {
            // Note: 32 is the max push constant ranges we can ever have because the Vulkan spec only guarantees 128
            // bytes with 4-byte alignment.
            if (config.pushConstantRangeCount > 32)
            {
                Logger::Error(
                    "[VULKAN_PIPELINE] - Create() Cannot have more than 32 push constant ranges. Passed count: {}",
                    config.pushConstantRangeCount);
                return false;
            }

            VkPushConstantRange ranges[32] = {};
            for (u32 i = 0; i < config.pushConstantRangeCount; i++)
            {
                ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                ranges[i].offset     = static_cast<u32>(config.pushConstantRanges[i].offset);
                ranges[i].size       = static_cast<u32>(config.pushConstantRanges[i].size);
            }

            pipelineLayoutCreateInfo.pushConstantRangeCount = config.pushConstantRangeCount;
            pipelineLayoutCreateInfo.pPushConstantRanges    = ranges;
        }
        else
        {
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
            pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
        }

        // Descriptor set layouts
        pipelineLayoutCreateInfo.setLayoutCount = config.descriptorSetLayoutCount;
        pipelineLayoutCreateInfo.pSetLayouts    = config.descriptorSetLayouts;

        // Create our pipeline layout
        VK_CHECK(vkCreatePipelineLayout(context->device.logicalDevice, &pipelineLayoutCreateInfo, context->allocator,
                                        &layout));

        VK_SET_DEBUG_OBJECT_NAME(context, VK_OBJECT_TYPE_PIPELINE_LAYOUT, layout,
                                 "PIPELINE_LAYOUT_" + config.shaderName);

        // Pipeline create info
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineCreateInfo.stageCount                   = config.stageCount;
        pipelineCreateInfo.pStages                      = config.stages;
        pipelineCreateInfo.pVertexInputState            = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState          = &inputAssembly;

        pipelineCreateInfo.pViewportState      = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState   = &multiSampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState  = (config.shaderFlags & ShaderFlagDepthTest) ? &depthStencil : nullptr;
        pipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
        pipelineCreateInfo.pTessellationState  = nullptr;

        pipelineCreateInfo.layout = layout;

        pipelineCreateInfo.renderPass         = config.renderPass->handle;
        pipelineCreateInfo.subpass            = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex  = -1;

        // Create our pipeline
        VkResult result = vkCreateGraphicsPipelines(context->device.logicalDevice, VK_NULL_HANDLE, 1,
                                                    &pipelineCreateInfo, context->allocator, &m_handle);

        VK_SET_DEBUG_OBJECT_NAME(context, VK_OBJECT_TYPE_PIPELINE, m_handle, "PIPELINE_" + config.shaderName);

        if (VulkanUtils::IsSuccess(result))
        {
            Logger::Debug("[VULKAN_PIPELINE] - Graphics pipeline created");
            return true;
        }

        Logger::Error("[VULKAN_PIPELINE] - vkCreateGraphicsPipelines failed with: {}",
                      VulkanUtils::ResultString(result, true));
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
        // Make sure to use the bound topology type
        vkCmdSetPrimitiveTopology(commandBuffer->handle, m_currentTopology);
    }
}  // namespace C3D
