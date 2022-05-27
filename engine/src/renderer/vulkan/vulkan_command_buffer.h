
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanCommandBufferManager
{
	void Allocate(VulkanContext* context, VkCommandPool pool, bool isPrimary, VulkanCommandBuffer* commandBuffer);

	void Free(const VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer);

	void Begin(VulkanCommandBuffer* commandBuffer, bool isSingleUse, bool isRenderPassContinue, bool isSimultaneousUse);

	void End(VulkanCommandBuffer* commandBuffer);

	void UpdateSubmitted(VulkanCommandBuffer* commandBuffer);

	void Reset(VulkanCommandBuffer* commandBuffer);

	void AllocateAndBeginSingleUse(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer);

	void EndSingleUse(const VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* commandBuffer, VkQueue queue);
}