
#pragma once
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"

#include "resources/material.h"

namespace C3D
{
	constexpr auto UI_SHADER_STAGE_COUNT = 2;
	constexpr auto VULKAN_UI_SHADER_DESCRIPTOR_COUNT = 2;
	constexpr auto VULKAN_UI_SHADER_SAMPLER_COUNT = 1;

	// TODO: make configurable
	constexpr auto VULKAN_MAX_UI_COUNT = 1024;

	struct VulkanUiShaderInstanceState
	{
		// Per frame
		VkDescriptorSet descriptorSets[3];

		// Per Descriptor
		VulkanDescriptorState descriptorStates[VULKAN_UI_SHADER_DESCRIPTOR_COUNT];
	};

	// This structure should be 256 bytes for certain Nvidia cards
	struct VulkanUiShaderGlobalUbo
	{
		mat4 projection;	// 64 bytes
		mat4 view;			// 64 bytes

		mat4 mat4Padding0;	// 64 reserved bytes
		mat4 mat4Padding1;	// 64 reserved bytes
	};

	// This structure should be 256 bytes
	struct VulkanUiShaderInstanceUbo
	{
		vec4 diffuseColor;	// 16 bytes
		vec4 vec4Reserved0;	// 16 bytes, reserved for future use
		vec4 vec4Reserved1;	// 16 bytes, reserved for future use
		vec4 vec4Reserved2;	// 16 bytes, reserved for future use

		mat4 mat4Padding0;	// 64 bytes for padding
		mat4 mat4Padding1;	// 64 bytes for padding
		mat4 mat4Padding2;	// 64 bytes for padding
	};

	class VulkanUiShader
	{
	public:
		VulkanUiShader();

		bool Create(const VulkanContext* context);

		void Destroy(const VulkanContext* context);

		void Use(const VulkanContext* context) const;

		void UpdateGlobalState(const VulkanContext* context, f32 deltaTime) const;

		void SetModel(const VulkanContext* context, mat4 model) const;
		void ApplyMaterial(const VulkanContext* context, Material* material);

		bool AcquireResources(const VulkanContext* context, Material* material);

		void ReleaseResources(const VulkanContext* context, Material* material);

		VulkanUiShaderGlobalUbo globalUbo;
	private:
		LoggerInstance m_logger;

		VulkanShaderStage m_stages[UI_SHADER_STAGE_COUNT];

		VkDescriptorPool		m_globalDescriptorPool;
		VkDescriptorSetLayout	m_globalDescriptorSetLayout;
		VkDescriptorSet			m_globalDescriptorSets[3];

		VulkanBuffer m_globalUniformBuffer;

		VkDescriptorPool m_objectDescriptorPool;
		VkDescriptorSetLayout m_objectDescriptorSetLayout;

		VulkanBuffer m_objectUniformBuffer;
		// TODO: Manage a free list of some kind here instead
		u32 m_objectUniformBufferIndex;

		TextureUse m_samplerUses[VULKAN_UI_SHADER_SAMPLER_COUNT];

		// TODO: Make dynamic
		VulkanUiShaderInstanceState m_instanceStates[VULKAN_MAX_UI_COUNT];

		VulkanPipeline m_pipeline;
	};
}
