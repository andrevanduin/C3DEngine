
#pragma once
#include <vulkan/vulkan.h>
#include "core/defines.h"

#include "vulkan_renderpass.h"

namespace C3D
{
	class VulkanFrameBuffer
	{
	public:
		VulkanFrameBuffer();

		void Create(const VulkanContext* context, VulkanRenderPass* renderPass, u32 width, u32 height, u32 attachmentCount, const VkImageView* attachments);

		void Destroy(const VulkanContext* context);

		VkFramebuffer handle;
	private:
		u32 m_attachmentCount;
		VkImageView* m_attachments;

		VulkanRenderPass* m_renderPass;
	};
}