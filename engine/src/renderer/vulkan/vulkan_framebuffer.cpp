
#include "vulkan_framebuffer.h"

#include "core/logger.h"
#include "core/memory.h"

namespace C3D
{
	void VulkanFrameBufferManager::Create(const VulkanContext* context, VulkanRenderPass* renderPass, const u32 width, const u32 height,
	                                      const u32 attachmentCount, const VkImageView* attachments, VulkanFrameBuffer* frameBuffer)
	{
		frameBuffer->attachments = Memory::Allocate<VkImageView>(attachmentCount, MemoryType::Renderer);
		for (u32 i = 0; i < attachmentCount; i++)
		{
			frameBuffer->attachments[i] = attachments[i];
		}

		frameBuffer->renderPass = renderPass;
		frameBuffer->attachmentCount = attachmentCount;

		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass->handle;
		createInfo.attachmentCount = attachmentCount;
		createInfo.pAttachments = frameBuffer->attachments;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(context->device.logicalDevice, &createInfo, context->allocator, &frameBuffer->handle));
	}

	void VulkanFrameBufferManager::Destroy(const VulkanContext* context, VulkanFrameBuffer* frameBuffer)
	{
		vkDestroyFramebuffer(context->device.logicalDevice, frameBuffer->handle, context->allocator);
		if (frameBuffer->attachments)
		{
			Memory::Free(frameBuffer->attachments, sizeof(VkImageView) * frameBuffer->attachmentCount, MemoryType::Renderer);
			frameBuffer->attachments = nullptr;
		}

		frameBuffer->handle = nullptr;
		frameBuffer->attachmentCount = 0;
		frameBuffer->renderPass = nullptr;
	}
}
