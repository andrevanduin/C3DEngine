
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanFrameBufferManager
{
	void Create(const VulkanContext* context, VulkanRenderPass* renderPass, u32 width, u32 height, u32 attachmentCount,
	            const VkImageView* attachments, VulkanFrameBuffer* frameBuffer);

	void Destroy(const VulkanContext* context, VulkanFrameBuffer* frameBuffer);
}