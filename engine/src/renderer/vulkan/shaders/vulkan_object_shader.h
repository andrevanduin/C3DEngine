
#pragma once
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_pipeline.h"

namespace C3D
{
	constexpr auto OBJECT_SHADER_STAGE_COUNT = 2;

	class VulkanObjectShader
	{
	public:
		bool Create(const VulkanContext* context);

		void Destroy(const VulkanContext* context);

		void Use(VulkanContext* context);

	private:
		VulkanShaderStage m_stages[OBJECT_SHADER_STAGE_COUNT];
		VulkanPipeline m_pipeline;
	};	
}
