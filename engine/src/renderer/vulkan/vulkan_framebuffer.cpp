
#include "vulkan_framebuffer.h"
#include "vulkan_types.h"

#include "core/logger.h"
#include "core/memory.h"

namespace C3D
{
	VulkanFrameBuffer::VulkanFrameBuffer()
		: handle(nullptr), m_attachmentCount(0), m_attachments(nullptr), m_renderPass(nullptr)
	{
	}

	void VulkanFrameBuffer::Create(const VulkanContext* context, VulkanRenderPass* renderPass, const u32 width, const u32 height,
	                               const u32 attachmentCount, const VkImageView* attachments)
	{
		m_attachments = Memory::Allocate<VkImageView>(attachmentCount, MemoryType::Renderer);
		for (u32 i = 0; i < attachmentCount; i++)
		{
			m_attachments[i] = attachments[i];
		}

		m_renderPass = renderPass;
		m_attachmentCount = attachmentCount;

		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass->handle;
		createInfo.attachmentCount = attachmentCount;
		createInfo.pAttachments = m_attachments;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(context->device.logicalDevice, &createInfo, context->allocator, &handle));
	}

	void VulkanFrameBuffer::Destroy(const VulkanContext* context)
	{
		vkDestroyFramebuffer(context->device.logicalDevice, handle, context->allocator);
		if (m_attachments)
		{
			Memory::Free(m_attachments, sizeof(VkImageView) * m_attachmentCount, MemoryType::Renderer);
			m_attachments = nullptr;
		}

		handle = nullptr;
		m_attachmentCount = 0;
		m_renderPass = nullptr;
	}
}
