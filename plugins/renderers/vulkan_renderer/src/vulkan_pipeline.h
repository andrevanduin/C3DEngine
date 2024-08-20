
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "resources/shaders/shader_types.h"
#include "vulkan_types.h"

namespace C3D
{
    struct VulkanPipelineConfig
    {
        VulkanRenderpass* renderpass;
        u32 stride;

        /** @brief Array of Vertex Input Attribute descriptions that are part of this pipeline. */
        DynamicArray<VkVertexInputAttributeDescription> attributes;
        /** @brief Array of DescriptorSet layouts that are part of this pipeline. */
        DynamicArray<VkDescriptorSetLayout> descriptorSetLayouts;
        /** @brief Array of Shader Stages Create infos that are part of this pipeline. */
        DynamicArray<VkPipelineShaderStageCreateInfo> stages;
        /** @brief Array of Push Constant ranges that are part of this pipeline. */
        DynamicArray<Range> pushConstantRanges;

        VkViewport viewport;
        VkRect2D scissor;

        FaceCullMode cullMode;
        ShaderFlagBits shaderFlags = ShaderFlags::ShaderFlagNone;

        u32 topologyTypes = 0;

        /** @brief The name of the shader that is associated with this pipeline. */
        String shaderName;
    };

    class VulkanPipeline
    {
    public:
        VulkanPipeline(PrimitiveTopologyTypeBits topologyTypeBits, RendererWinding winding = RendererWinding::CounterClockwise);

        bool Create(const VulkanContext* context, const VulkanPipelineConfig& config);

        void Destroy();

        void Bind(const VulkanCommandBuffer* commandBuffer, VkPipelineBindPoint bindPoint) const;

        PrimitiveTopologyTypeBits GetSupportedTopologyTypes() const { return m_supportedTopologyTypes; }

        VkPipelineLayout layout = nullptr;

    private:
        VkPipeline m_handle = nullptr;
        VkPrimitiveTopology m_currentTopology;

        PrimitiveTopologyTypeBits m_supportedTopologyTypes = 0;
        RendererWinding m_rendererWinding;

        const VulkanContext* m_context;
    };
}  // namespace C3D