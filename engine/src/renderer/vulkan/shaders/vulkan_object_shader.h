
#pragma once
#include "renderer/renderer_types.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"

namespace C3D
{
	constexpr auto OBJECT_SHADER_STAGE_COUNT = 2;
	constexpr auto VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT = 2;
	constexpr auto VULKAN_OBJECT_MAX_OBJECT_COUNT = 1024;

	struct VulkanDescriptorState
	{
		u32 generations[3];
	};

	struct VulkanObjectShaderObjectState
	{
		VkDescriptorSet descriptorSets[3];

		VulkanDescriptorState descriptorStates[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
	};

	class VulkanObjectShader
	{
	public:
		VulkanObjectShader();

		bool Create(const VulkanContext* context, Texture* defaultDiffuseTexture);

		void Destroy(const VulkanContext* context);

		void Use(const VulkanContext* context) const;

		void UpdateGlobalState(const VulkanContext* context, f32 deltaTime) const;

		void UpdateObject(const VulkanContext* context, GeometryRenderData data);

		bool AcquireResources(const VulkanContext* context, u32* outObjectId);

		void ReleaseResources(const VulkanContext* context, u32 objectId);
		
		GlobalUniformObject globalUbo;
	private:
		VulkanShaderStage m_stages[OBJECT_SHADER_STAGE_COUNT];

		VkDescriptorPool		m_globalDescriptorPool;
		VkDescriptorSetLayout	m_globalDescriptorSetLayout;
		VkDescriptorSet			m_globalDescriptorSets[3];

		VulkanBuffer m_globalUniformBuffer;

		VkDescriptorPool m_objectDescriptorPool;
		VkDescriptorSetLayout m_objectDescriptorSetLayout;

		VulkanBuffer m_objectUniformBuffer;
		// TODO: Manage a free list of some kind here instead
		u32 m_objectUniformBufferIndex;

		// TODO: Make dynamic
		VulkanObjectShaderObjectState m_objectStates[VULKAN_OBJECT_MAX_OBJECT_COUNT];

		// Pointers to default textures
		Texture* m_defaultDiffuse;

		VulkanPipeline m_pipeline;
	};	
}
