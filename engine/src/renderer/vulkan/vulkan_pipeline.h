
#pragma once
#include "vulkan_types.h"

namespace C3D
{
	struct VulkanPipelineConfig
	{
		VulkanRenderPass* renderPass;
		u32 stride;

		u32 attributeCount;
		VkVertexInputAttributeDescription* attributes;

		u32 descriptorSetLayoutCount;
		VkDescriptorSetLayout* descriptorSetLayouts;

		u32 stageCount;
		VkPipelineShaderStageCreateInfo* stages;

		VkViewport viewport;
		VkRect2D scissor;

		FaceCullMode cullMode;
		bool isWireFrame;
		u32 shaderFlags;

		u32 pushConstantRangeCount;
		Range* pushConstantRanges;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline();

		bool Create(const VulkanContext* context, const VulkanPipelineConfig& config);

		void Destroy(const VulkanContext* context);

		void Bind(const VulkanCommandBuffer* commandBuffer, VkPipelineBindPoint bindPoint) const;

		VkPipelineLayout layout;
	private:
		VkPipeline m_handle;
	};
}