
#pragma once
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"
#include "core/defines.h"
#include "vulkan_types.h"

namespace C3D
{
	// Forward declare texture
	struct Texture;
	struct TextureMap;

	/* @brief The index of the global descriptor set. */
	constexpr auto DESC_SET_INDEX_GLOBAL = 0;
	/* @brief The index of the instance descriptor set. */
	constexpr auto DESC_SET_INDEX_INSTANCE = 1;

	/* @brief The index of the UBO binding. */
	constexpr auto BINDING_INDEX_UBO = 0;
	/* @brief The index of the image sampler binding. */
	constexpr auto BINDING_INDEX_SAMPLER = 1;

	/* @brief The maximum number of stages (such as vertex, fragment, compute etc.) allowed in a shader. */
	constexpr auto VULKAN_SHADER_MAX_STAGES = 8;
	/* @brief The maximum number of textures allowed at global level.  */
	constexpr auto VULKAN_SHADER_MAX_GLOBAL_TEXTURES = 31;
	/* @brief The maximum number of textures allowed at instance level.  */
	constexpr auto VULKAN_SHADER_MAX_INSTANCE_TEXTURES = 31;
	/* @brief The maximum number of vertex input attributes allowed . */
	constexpr auto VULKAN_SHADER_MAX_ATTRIBUTES = 16;
	/*
	 * @brief The maximum number of uniforms and samplers allowed at the global, instance and local levels combined.
	 * this is probably more than we will ever need.
	 */
	constexpr auto VULKAN_SHADER_MAX_UNIFORMS = 128;
	/* @brief The maximum number of bindings per descriptor set. */
	constexpr auto VULKAN_SHADER_MAX_BINDINGS = 32;
	/* @brief The maximum number of push constant ranges for a shader. */
	constexpr auto VULKAN_SHADER_MAX_PUSH_CONST_RANGES = 32;

	// TODO: make configurable
	constexpr auto VULKAN_MAX_MATERIAL_COUNT = 1024;

	constexpr auto VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH = 255;

	/* @brief small structure containing Vulkan format and corresponding size in bytes. */
	struct VulkanFormatSize
	{
		VkFormat format;
		u8 size;
	};

	/* Configuration for a specific shader stage. */
	struct VulkanShaderStageConfig
	{
		VkShaderStageFlagBits stage;
		char fileName[VULKAN_SHADER_STAGE_CONFIG_FILENAME_MAX_LENGTH];
	};

	/* Configuration for a specific descriptor set. */
	struct VulkanDescriptorSetConfig
	{
		u8 bindingCount;
		VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
	};

	struct VulkanShaderConfig
	{
		/* @brief The number of stages in this shader. */
		u8 stageCount;
		/* @brief The configuration for every stage. */
		VulkanShaderStageConfig stages[VULKAN_SHADER_MAX_STAGES];
		/* @brief An array of the descriptor pool sizes. */
		VkDescriptorPoolSize poolSizes[2];
		/*
		 * @brief The max number of descriptor sets that can be allocated from this shader.
		 * Typically should be a decently high number
		 */
		u16 maxDescriptorSetCount;
		/*
		 * @brief Total number of descriptor sets configured for this shader.
		 * This will be 1 if only using global uniforms/samplers; otherwise it will be 2.
		 */
		u8 descriptorSetCount;
		/* @brief The Descriptor sets, Index 0=global and 1=instance. */
		VulkanDescriptorSetConfig descriptorSets[2];
		/* @brief An array of attribute descriptions for this shader. */
		VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
	};

	struct VulkanDescriptorState
	{
		/* @brief the descriptor generation, one per frame. */
		u8 generations[3];
		/* The identifier, one per frame. Typically for texture ids. */
		u32 ids[3];
	};

	struct VulkanShaderDescriptorSetState
	{
		/* @brief The descriptor sets for this instance, one per frame. */
		VkDescriptorSet descriptorSets[3];
		/* @brief A descriptor state per descriptor, which in turn handles frames. Count for this is managed in the shader config. */
		VulkanDescriptorState descriptorStates[VULKAN_SHADER_MAX_BINDINGS];
	};

	struct VulkanShaderInstanceState
	{
		/* @brief Instance id. INVALID_ID if not used. */
		u32 id;
		/* @brief The offset in bytes in the instance uniform buffer. */
		u64 offset;
		/* @brief A state for the descriptor set. */
		VulkanShaderDescriptorSetState descriptorSetState;
		/* @brief Instance texture map pointers, which are used during rendering.
		 * These are set by calls to SetSampler.
		 */
		Texture** instanceTextures;
	};

	struct VulkanShaderStage
	{
		VkShaderModuleCreateInfo createInfo;
		VkShaderModule handle;
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
	};

	struct VulkanShader
	{
		void* mappedUniformBufferBlock;

		VulkanShaderConfig config;

		VulkanRenderPass* renderPass;

		VulkanShaderStage stages[VULKAN_SHADER_MAX_STAGES];

		VkDescriptorPool descriptorPool;

		VkDescriptorSetLayout descriptorSetLayouts[2];
		VkDescriptorSet globalDescriptorSets[3];

		VulkanBuffer uniformBuffer;

		VulkanPipeline pipeline;

		u32 instanceCount;
		VulkanShaderInstanceState instanceStates[VULKAN_MAX_MATERIAL_COUNT];
	};
}