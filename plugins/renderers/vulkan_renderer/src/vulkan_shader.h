
#pragma once
#include <core/defines.h>
#include <resources/shaders/shader_types.h>

#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_types.h"

namespace C3D
{
    struct Texture;
    struct TextureMap;

    /** @brief The index of the global descriptor set. */
    constexpr auto DESC_SET_INDEX_GLOBAL = 0;
    /** @brief The index of the instance descriptor set. */
    constexpr auto DESC_SET_INDEX_INSTANCE = 1;

    /** @brief The maximum number of stages (such as vertex, fragment, compute etc.) allowed in a shader. */
    constexpr auto VULKAN_SHADER_MAX_STAGES = 8;
    /** @brief The maximum number of textures allowed at global level.  */
    constexpr auto VULKAN_SHADER_MAX_GLOBAL_TEXTURES = 31;
    /** @brief The maximum number of textures allowed at instance level.  */
    constexpr auto VULKAN_SHADER_MAX_INSTANCE_TEXTURES = 31;
    /** @brief The maximum number of vertex input attributes allowed . */
    constexpr auto VULKAN_SHADER_MAX_ATTRIBUTES = 16;
    /**
     * @brief The maximum number of uniforms and samplers allowed at the global, instance and local levels combined.
     * this is probably more than we will ever need.
     */
    constexpr auto VULKAN_SHADER_MAX_UNIFORMS = 128;
    /** @brief The maximum number of bindings per descriptor set. */
    constexpr auto VULKAN_SHADER_MAX_BINDINGS = 2;
    /** @brief The maximum number of push constant ranges for a shader. */
    constexpr auto VULKAN_SHADER_MAX_PUSH_CONST_RANGES = 32;
    /** @brief The maximum number of topology classes that our shader can have. */
    constexpr auto VULKAN_MAX_NUMBER_OF_TOPOLOGY_CLASSES = 3;

    // TODO: make configurable
    constexpr auto VULKAN_MAX_MATERIAL_COUNT = 1024;

    constexpr auto VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH = 255;

    /** @brief small structure containing Vulkan format and corresponding size in bytes. */
    struct VulkanFormatSize
    {
        VkFormat format;
        u8 size;
    };

    /** @brief Configuration for a specific shader stage. */
    struct VulkanShaderStageConfig
    {
        VkShaderStageFlagBits stage;
        String fileName;
    };

    /** @brief Configuration for a specific descriptor set. */
    struct VulkanDescriptorSetConfig
    {
        /** @brief The number of bindings in this set. */
        u8 bindingCount;
        /** @brief An array of binding layouts for this set. */
        VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
        /** @brief The index of the sampler binding . */
        u8 samplerBindingIndex;
    };

    struct VulkanShaderConfig
    {
        /** @brief The number of stages in this shader. */
        u8 stageCount;
        /** @brief The configuration for every stage. */
        VulkanShaderStageConfig stages[VULKAN_SHADER_MAX_STAGES];
        /* @brief An array of the descriptor pool sizes. */
        VkDescriptorPoolSize poolSizes[2];
        /**
         * @brief The max number of descriptor sets that can be allocated from this shader.
         * Typically should be a decently high number
         */
        u16 maxDescriptorSetCount;
        /**
         * @brief Total number of descriptor sets configured for this shader.
         * This will be 1 if only using global uniforms/samplers; otherwise it will be 2.
         */
        u8 descriptorSetCount;
        /** @brief The Descriptor sets, Index 0=global and 1=instance. */
        VulkanDescriptorSetConfig descriptorSets[2];
        /** @brief An array of attribute descriptions for this shader. */
        VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
        /** @brief Face culling mode, provided by the front end. */
        FaceCullMode cullMode;
        /** @brief Topology types used by the shader. */
        PrimitiveTopologyTypeBits topologyTypes;
    };

    struct VulkanDescriptorState
    {
        /** @brief the descriptor generation, one per frame. */
        u8 generations[3];
        /** The identifier, one per frame. Typically for texture ids. */
        u32 ids[3];
    };

    struct VulkanShaderDescriptorSetState
    {
        /** @brief The descriptor sets for this instance, one per frame. */
        VkDescriptorSet descriptorSets[3];
        /** @brief A descriptor state per descriptor, which in turn handles frames. Count for this is managed in the
         * shader config. */
        VulkanDescriptorState descriptorStates[VULKAN_SHADER_MAX_BINDINGS];
    };

    struct VulkanShaderInstanceState
    {
        /** @brief Instance id. INVALID_ID if not used. */
        u32 id;
        /* @brief The offset in bytes in the instance uniform buffer. */
        u64 offset;
        /** @brief A state for the descriptor set. */
        VulkanShaderDescriptorSetState descriptorSetState;
        /** @brief Instance texture map pointers, which are used during rendering.
         * These are set by calls to SetSampler.
         */
        TextureMap** instanceTextureMaps;
    };

    struct VulkanShaderStage
    {
        VkShaderModuleCreateInfo createInfo;
        VkShaderModule handle;
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
    };

    struct VulkanShader
    {
        explicit VulkanShader(const VulkanContext* context) : uniformBuffer(context, "GLOBAL_UNIFORM") {}

        void* mappedUniformBufferBlock = nullptr;

        VulkanShaderConfig config;

        VulkanRenderPass* renderPass = nullptr;

        VulkanShaderStage stages[VULKAN_SHADER_MAX_STAGES];

        VkDescriptorPool descriptorPool = nullptr;

        VkDescriptorSetLayout descriptorSetLayouts[2];
        VkDescriptorSet globalDescriptorSets[3];

        VulkanBuffer uniformBuffer;

        /** @brief An array of pipelines used by this shader. */
        DynamicArray<VulkanPipeline*> pipelines;
        /** @brief An array of pipelines used by this shader for clockwise topology.
         * (Only used when system does not support dynamic winding natively or by extension)*/
        DynamicArray<VulkanPipeline*> clockwisePipelines;

        u8 boundPipeline = INVALID_ID_U8;

        u32 instanceCount = 0;
        VulkanShaderInstanceState instanceStates[VULKAN_MAX_MATERIAL_COUNT];

        void ZeroOutCounts()
        {
            globalUniformCount          = 0;
            globalUniformSamplerCount   = 0;
            instanceUniformCount        = 0;
            instanceUniformSamplerCount = 0;
            localUniformCount           = 0;
        }

        /** @brief The number of global non-sampler uniforms. */
        u8 globalUniformCount = 0;
        /** @brief The number of global sampler uniforms. */
        u8 globalUniformSamplerCount = 0;
        /** @brief The number of instance non-sampler uniforms. */
        u8 instanceUniformCount = 0;
        /** @brief The number of instance sampler uniforms. */
        u8 instanceUniformSamplerCount = 0;
        /** @brief The number of local non-sampler uniforms. */
        u8 localUniformCount = 0;
    };

    static const char* ToString(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                return "VERTEX";
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return "FRAGMENT";
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return "COMPUTE";
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                return "GEOMETRY";
            default:
                return "UNKNOWN";
        }
    }
}  // namespace C3D