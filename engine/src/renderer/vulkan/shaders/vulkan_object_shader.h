
#pragma once
#include "renderer/renderer_types.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"

namespace C3D
{
	constexpr auto OBJECT_SHADER_STAGE_COUNT = 2;

	class VulkanObjectShader
	{
	public:
		VulkanObjectShader();

		bool Create(const VulkanContext* context);

		void Destroy(const VulkanContext* context);

		void Use(const VulkanContext* context) const;

		void UpdateGlobalState(const VulkanContext* context) const;

		void UpdateObject(const VulkanContext* context, mat4 model) const;

		GlobalUniformObject globalUbo;
	private:
		VulkanShaderStage m_stages[OBJECT_SHADER_STAGE_COUNT];

		VkDescriptorPool		m_globalDescriptorPool;
		VkDescriptorSetLayout	m_globalDescriptorSetLayout;
		VkDescriptorSet			m_globalDescriptorSets[3];

		VulkanBuffer m_globalUniformBuffer;

		VulkanPipeline m_pipeline;
	};	
}
