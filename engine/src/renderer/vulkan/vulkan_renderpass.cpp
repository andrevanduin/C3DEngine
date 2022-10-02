
#include "vulkan_renderpass.h"
#include "vulkan_types.h"

#include "core/logger.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	VulkanRenderPass::VulkanRenderPass()
		: RenderPass(), handle(nullptr), state(), m_depth(0), m_stencil(0), m_context(nullptr)
	{}

	VulkanRenderPass::VulkanRenderPass(VulkanContext* context, const u16 _id, const RenderPassConfig& config)
		: RenderPass(_id, config), handle(nullptr), state(), m_depth(0), m_stencil(0), m_context(context)
	{}

	void VulkanRenderPass::Create(f32 depth, u32 stencil)
	{
		m_depth = depth;
		m_stencil = stencil;

		// Main SubPass
		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// TODO: Make this configurable.
		u32 attachmentDescriptionCount = 0;
		VkAttachmentDescription attachmentDescriptions[2];

		// Color attachment
		bool doClearColor = m_clearFlags & ClearColor;

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_context->swapChain.imageFormat.format; // TODO: Make this configurable.
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// If the user has requested clear color we clear otherwise we load it
		colorAttachment.loadOp = doClearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// If we have a previous pass, the initialLayout should already be set. Otherwise it will be undefined.
		colorAttachment.initialLayout = m_hasPrevPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		// If there are more passes we use the optimal layout. Otherwise we want to have a present layout
		colorAttachment.finalLayout = m_hasNextPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.flags = 0;

		attachmentDescriptions[attachmentDescriptionCount] = colorAttachment;
		attachmentDescriptionCount++;

		VkAttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subPass.colorAttachmentCount = 1;
		subPass.pColorAttachments = &colorAttachmentReference;

		// Depth attachment, if there is one.
		if (bool doClearDepth = m_clearFlags & ClearDepth)
		{
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = m_context->device.depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			if (m_hasPrevPass)
			{
				depthAttachment.loadOp = doClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			}
			else
			{
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachmentDescriptions[attachmentDescriptionCount] = depthAttachment;
			attachmentDescriptionCount++;

			VkAttachmentReference depthAttachmentReference;
			depthAttachmentReference.attachment = 1;
			depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// TODO: other attachment types (input, resolve, preserve)

			subPass.pDepthStencilAttachment = &depthAttachmentReference;
		}
		else
		{
			Memory.Zero(&attachmentDescriptions[attachmentDescriptionCount], sizeof(VkAttachmentDescription));
			subPass.pDepthStencilAttachment = nullptr;
		}

		// Input from a Shader.
		subPass.inputAttachmentCount = 0;
		subPass.pInputAttachments = nullptr;

		// Attachments used for multi-sampling color attachments.
		subPass.pResolveAttachments = nullptr;

		// Attachments not used in this subPass, but must be preserved for the next.
		subPass.preserveAttachmentCount = 0;
		subPass.preserveAttachmentCount = 0;

		// RenderPass dependencies
		VkSubpassDependency dependency;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;

		// RenderPass Create
		VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = attachmentDescriptionCount;
		createInfo.pAttachments = attachmentDescriptions;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;

		VK_CHECK(vkCreateRenderPass(m_context->device.logicalDevice, &createInfo, m_context->allocator, &handle));

		Logger::Info("[VULKAN_RENDER_PASS] - RenderPass successfully created");
	}

	void VulkanRenderPass::Destroy()
	{
		Logger::Info("[VULKAN_RENDER_PASS] - Destroying RenderPass");

		if (handle)
		{
			vkDestroyRenderPass(m_context->device.logicalDevice, handle, m_context->allocator);
			handle = nullptr;
		}
	}

	void VulkanRenderPass::Begin(VulkanCommandBuffer* commandBuffer, const RenderTarget* target) const
	{
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = handle;
		beginInfo.framebuffer = static_cast<VkFramebuffer>(target->internalFrameBuffer);
		beginInfo.renderArea.offset.x = renderArea.x;
		beginInfo.renderArea.offset.y = renderArea.y;
		beginInfo.renderArea.extent.width = renderArea.z;
		beginInfo.renderArea.extent.height = renderArea.w;

		beginInfo.clearValueCount = 0;
		beginInfo.pClearValues = nullptr;

		VkClearValue clearValues[2];
		Memory.Zero(clearValues, sizeof(VkClearValue) * 2);

		if (m_clearFlags & ClearColor)
		{
			Memory.Copy(clearValues[beginInfo.clearValueCount].color.float32, &m_clearColor, sizeof(f32) * 4);
		}
		beginInfo.clearValueCount++;

		if (m_clearFlags & ClearDepth)
		{
			Memory.Copy(clearValues[beginInfo.clearValueCount].color.float32, &m_clearColor, sizeof(f32) * 4);
			clearValues[beginInfo.clearValueCount].depthStencil.depth = m_depth;

			const bool doClearStencil = m_clearFlags & ClearStencil;
			clearValues[beginInfo.clearValueCount].depthStencil.stencil = doClearStencil ? m_stencil : 0;
			beginInfo.clearValueCount++;
		}

		beginInfo.pClearValues = beginInfo.clearValueCount > 0 ? clearValues : nullptr;

		vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		commandBuffer->state = VulkanCommandBufferState::InRenderPass;
	}

	void VulkanRenderPass::End(VulkanCommandBuffer* commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer->handle);
		commandBuffer->state = VulkanCommandBufferState::Recording;
	}
}
