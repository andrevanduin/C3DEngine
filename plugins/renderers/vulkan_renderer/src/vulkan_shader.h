
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
    /** @brief The maximum number of push constant ranges for a shader. */
    constexpr auto VULKAN_SHADER_MAX_PUSH_CONST_RANGES = 32;
    /** @brief The maximum number of topology classes that our shader can have. */
    constexpr auto VULKAN_MAX_NUMBER_OF_TOPOLOGY_CLASSES = 3;

    constexpr auto VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH = 255;

    /** @brief small structure containing Vulkan format and corresponding size in bytes. */
    struct VulkanFormatSize
    {
        VkFormat format;
        u8 size;
    };

    /** @brief Configuration for a specific descriptor set. */
    struct VulkanDescriptorSetConfig
    {
        /** @brief The number of bindings in this set. */
        u8 bindingCount;
        /** @brief An array of binding layouts for this set. */
        DynamicArray<VkDescriptorSetLayoutBinding> bindings;
        /** @brief The starting index of the sampler binding . */
        u8 samplerBindingIndexStart;
    };

    struct VulkanDescriptorState
    {
        /** @brief the descriptor generation, one per frame. */
        u8 generations[3];
        /** The identifier, one per frame. Typically for texture ids. */
        u32 ids[3];
    };

    struct VulkanUniformSamplerState
    {
        const ShaderUniform* uniform = nullptr;
        DynamicArray<TextureMap*> textureMaps;
        DynamicArray<VulkanDescriptorState> descriptorStates;
    };

    struct VulkanShaderInstanceState
    {
        /** @brief Instance id. INVALID_ID if not used. */
        u32 id = INVALID_ID;
        /** @brief The offset in bytes in the instance uniform buffer. */
        u64 offset = 0;
        /** @brief The descriptor sets for this instance, one per frame. */
        // TODO: handle frame count != 3
        VkDescriptorSet descriptorSets[3];
        /** @brief Ubo descriptor state. */
        VulkanDescriptorState uboDescriptorState;
        /** @brief A mapping of sampler uniforms to descriptors and texture maps. */
        DynamicArray<VulkanUniformSamplerState> samplerUniforms;
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

        /** @brief The block of memory mapped to the uniform buffer. */
        void* mappedUniformBufferBlock = nullptr;
        /** @brief The block of memory used for push constants (128 bytes). */
        void* localPushConstantBlock = nullptr;
        /** @brief The identifier for this shader. */
        u32 id = INVALID_ID;
        /** @brief The max number of descriptor sets that can be allocated from this shader. */
        u16 maxDescriptorSetCount = 0;
        /** @brief The total number of descriptor sets configured for this shader.
         * This value is 1 if this shader is only using global uniforms and sampler, otherwise it's 2. */
        u8 descriptorSetCount = 0;
        /** @brief The descriptor sets for this shader. Max of 2 (0 == global and 1 == instance). */
        VulkanDescriptorSetConfig descriptorSets[2];
        /** @brief An array of attribute descriptions for this shader. */
        VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
        /** @brief The face culling mode used by this shader. */
        FaceCullMode cullMode;
        /** @brief The maximum number of instances supported by this shader. */
        u32 maxInstances = 1;
        /** @brief A pointer to the renderpass used by this shader. */
        VulkanRenderpass* renderpass = nullptr;
        /** @brief The number of stages (Vertex, Fragment etc.) for this shader. */
        u8 stageCount;
        /** @brief The stages (Vertex, Fragment etc.) for this shader. */
        VulkanShaderStage stages[VULKAN_SHADER_MAX_STAGES];
        /** @brief The number of descriptor pool sizes. */
        u8 descriptorPoolSizeCount = 0;
        /** @brief The size of the descriptor pool used by this shader. */
        VkDescriptorPoolSize descriptorPoolSizes[2];
        /** @brief The descriptor pool used by this shader. */
        VkDescriptorPool descriptorPool;
        /** @brief The descriptor set layouts. Max of 2 (0 == global and 1 == instance). */
        VkDescriptorSetLayout descriptorSetLayouts[2];
        /** @brief Global descriptor sets. One per frame. */
        // TODO: Handle frame counts != 3
        VkDescriptorSet globalDescriptorSets[3];
        /** @brief The shader's UBO descriptor state. */
        VulkanDescriptorState globalUboDescriptorState;
        /** @brief A mapping of sampler uniforms to descriptors and texture maps. */
        DynamicArray<VulkanUniformSamplerState> globalSamplerUniforms;
        /** @brief The uniform buffer used by this shader. */
        VulkanBuffer uniformBuffer;
        /** @brief An array of pipelines used by this shader. */
        DynamicArray<VulkanPipeline*> pipelines;
        /** @brief An array of wireframe pipelines used by this shader.*/
        DynamicArray<VulkanPipeline*> wireframePipelines;
        /** @brief The index of the currently bound pipeline. */
        u8 boundPipelineIndex = INVALID_ID_U8;
        /** @brief The currently selected topology. */
        VkPrimitiveTopology currentTopology;
        /** @brief The instance states for all instances. */
        DynamicArray<VulkanShaderInstanceState> instanceStates;
    };

    static constexpr const char* ToString(VkShaderStageFlagBits stage)
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