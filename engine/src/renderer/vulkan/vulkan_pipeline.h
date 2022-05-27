
#pragma once
#include "vulkan_types.h"

namespace C3D
{
	class VulkanPipeline
	{
	public:
		bool Create(const VulkanContext* context, const VulkanRenderPass* renderPass, u32 attributeCount, VkVertexInputAttributeDescription* attributes,
			u32 descriptorSetLayoutCount, VkDescriptorSetLayout* descriptorSetLayouts, u32 stageCount, VkPipelineShaderStageCreateInfo* stages,
			VkViewport viewport, VkRect2D scissor, bool isWireFrame);

		void Destroy(const VulkanContext* context);

		void Bind(const VulkanCommandBuffer* commandBuffer, VkPipelineBindPoint bindPoint) const;

	private:
		VkPipeline m_handle;
		VkPipelineLayout m_layout;
	};
}