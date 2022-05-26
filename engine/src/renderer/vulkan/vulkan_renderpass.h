
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanRenderPassManager
{
	void Create(VulkanContext* context, VulkanRenderPass* renderPass, f32 x, f32 y, f32 w, f32 h, 
		f32 r, f32 g, f32 b, f32 a, f32 depth, u32 stencil);

	void Destroy(const VulkanContext* context, VulkanRenderPass* renderPass);

	void Begin(VulkanCommandBuffer* commandBuffer, const VulkanRenderPass* renderPass, VkFramebuffer frameBuffer);

	void End(VulkanCommandBuffer* commandBuffer, VulkanRenderPass* renderPass);
}