
#pragma once
#include "renderer/renderer_types.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"

#include "resources/material.h"

namespace C3D
{
	constexpr auto VULKAN_MATERIAL_SHADER_STAGE_COUNT = 2;
	constexpr auto VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT = 2;
	constexpr auto VULKAN_MATERIAL_SHADER_SAMPLER_COUNT = 1;

	// Max number of material instances
	// TODO: make configurable
	constexpr auto VULKAN_MAX_MATERIAL_COUNT = 1024;

	// Max number of simultaneously uploaded geometries
	// TODO: make configurable
	constexpr auto VULKAN_MAX_GEOMETRY_COUNT = 4096;

	struct VulkanDescriptorState
	{
		// Per frame
		u32 generations[3];
		u32 textureIds[3];
	};

	struct VulkanMaterialShaderInstanceState
	{
		// Per Frame
		VkDescriptorSet descriptorSets[3];
		// Per Descriptor
		VulkanDescriptorState descriptorStates[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
	};

	struct VulkanGeometryData
	{
		u32 id;
		u32 generation;

		u32 vertexCount;
		u32 vertexSize;
		u32 vertexBufferOffset;

		u32 indexCount;
		u32 indexSize;
		u32 indexBufferOffset;
	};

	class VulkanMaterialShader
	{
	public:
		VulkanMaterialShader();

		bool Create(const VulkanContext* context);

		void Destroy(const VulkanContext* context);

		void Use(const VulkanContext* context) const;

		void UpdateGlobalState(const VulkanContext* context, f32 deltaTime) const;

		void SetModel(const VulkanContext* context, mat4 model) const;
		void ApplyMaterial(const VulkanContext* context, Material* material);

		bool AcquireResources(const VulkanContext* context, Material* material);

		void ReleaseResources(const VulkanContext* context, Material* material);
		
		GlobalUniformObject globalUbo;
	private:
		VulkanShaderStage m_stages[VULKAN_MATERIAL_SHADER_STAGE_COUNT];

		VkDescriptorPool		m_globalDescriptorPool;
		VkDescriptorSetLayout	m_globalDescriptorSetLayout;
		VkDescriptorSet			m_globalDescriptorSets[3];

		VulkanBuffer m_globalUniformBuffer;

		VkDescriptorPool m_objectDescriptorPool;
		VkDescriptorSetLayout m_objectDescriptorSetLayout;

		VulkanBuffer m_objectUniformBuffer;
		// TODO: Manage a free list of some kind here instead
		u32 m_objectUniformBufferIndex;

		TextureUse m_samplerUses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

		// TODO: Make dynamic
		VulkanMaterialShaderInstanceState m_instanceStates[VULKAN_MAX_MATERIAL_COUNT];

		VulkanPipeline m_pipeline;
	};	
}
