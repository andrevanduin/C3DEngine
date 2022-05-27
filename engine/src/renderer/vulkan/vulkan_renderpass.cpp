
#include "vulkan_renderpass.h"
#include "core/memory.h"
#include "core/logger.h"

namespace C3D
{
	void VulkanRenderPassManager::Create(VulkanContext* context, VulkanRenderPass* renderPass, f32 x, f32 y, f32 w, f32 h,
		f32 r, f32 g, f32 b, f32 a, f32 depth, u32 stencil)
	{
		Logger::PushPrefix("VULKAN_RENDER_PASS");

		renderPass->x = static_cast<i32>(x);
		renderPass->y = static_cast<i32>(y);
		renderPass->w = static_cast<i32>(w);
		renderPass->h = static_cast<i32>(h);

		renderPass->r = r;
		renderPass->g = g;
		renderPass->b = b;
		renderPass->a = a;

		renderPass->depth = depth;
		renderPass->stencil = stencil;

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		constexpr u32 attachmentDescriptionCount = 2; // TODO: Make this configurable.
		VkAttachmentDescription attachmentDescriptions[attachmentDescriptionCount];

		VkAttachmentDescription colorAttachment;
		colorAttachment.format = context->swapChain.imageFormat.format; // TODO: Make this configurable.
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.flags = 0;

		attachmentDescriptions[0] = colorAttachment;

		VkAttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subPass.colorAttachmentCount = 1;
		subPass.pColorAttachments = &colorAttachmentReference;

		VkAttachmentDescription depthAttachment;
		depthAttachment.format = context->device.depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.flags = 0;

		attachmentDescriptions[1] = depthAttachment;

		VkAttachmentReference depthAttachmentReference;
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// TODO: other attachment types (input, resolve, preserve)

		subPass.pDepthStencilAttachment = &depthAttachmentReference;

		// Input from a Shader.
		subPass.inputAttachmentCount = 0;
		subPass.pInputAttachments = nullptr;

		// Attachments used for multi-sampling color attachments.
		subPass.pResolveAttachments = nullptr;

		// Attachments not used in this subPass, but must be preserved for the next.
		subPass.preserveAttachmentCount = 0;
		subPass.preserveAttachmentCount = 0;

		VkSubpassDependency dependency;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;

		VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = attachmentDescriptionCount;
		createInfo.pAttachments = attachmentDescriptions;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;

		VK_CHECK(vkCreateRenderPass(context->device.logicalDevice, &createInfo, context->allocator, &renderPass->handle));

		Logger::Info("RenderPass successfully created");
		Logger::PopPrefix();
	}

	void VulkanRenderPassManager::Destroy(const VulkanContext* context, VulkanRenderPass* renderPass)
	{
		Logger::PrefixInfo("VULKAN_RENDER_PASS", "Destroying RenderPass");

		if (renderPass && renderPass->handle)
		{
			vkDestroyRenderPass(context->device.logicalDevice, renderPass->handle, context->allocator);
			renderPass->handle = nullptr;
		}
	}

	void VulkanRenderPassManager::Begin(VulkanCommandBuffer* commandBuffer, const VulkanRenderPass* renderPass, VkFramebuffer frameBuffer)
	{
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass->handle;
		beginInfo.framebuffer = frameBuffer;
		beginInfo.renderArea.offset.x = renderPass->x;
		beginInfo.renderArea.offset.y = renderPass->y;
		beginInfo.renderArea.extent.width = renderPass->w;
		beginInfo.renderArea.extent.height = renderPass->h;

		VkClearValue clearValues[2];
		Memory::Zero(clearValues, sizeof(VkClearValue) * 2);

		clearValues[0].color.float32[0] = renderPass->r;
		clearValues[0].color.float32[1] = renderPass->g;
		clearValues[0].color.float32[2] = renderPass->b;
		clearValues[0].color.float32[3] = renderPass->a;

		clearValues[1].depthStencil.depth = renderPass->depth;
		clearValues[1].depthStencil.stencil = renderPass->stencil;

		beginInfo.clearValueCount = 2;
		beginInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		commandBuffer->state = VulkanCommandBufferState::InRenderPass;
	}

	void VulkanRenderPassManager::End(VulkanCommandBuffer* commandBuffer, VulkanRenderPass* renderPass)
	{
		vkCmdEndRenderPass(commandBuffer->handle);
		commandBuffer->state = VulkanCommandBufferState::Recording;
	}
}
