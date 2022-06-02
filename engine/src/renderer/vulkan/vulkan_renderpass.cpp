
#include "vulkan_renderpass.h"
#include "vulkan_types.h"

#include "core/logger.h"
#include "services/services.h"

namespace C3D
{
	VulkanRenderPass::VulkanRenderPass()
		: handle(nullptr), state(), area(), m_clearColor(), m_depth(0), m_stencil(0), m_clearFlags(0),
		  m_hasPrevPass(false), m_hasNextPass(false)
	{
	}

	void VulkanRenderPass::Create(VulkanContext* context, const ivec4& renderArea, const vec4& clearColor, f32 depth, u32 stencil, u8 clearFlags, bool hasPrevPass, bool hasNextPass)
	{
		area = renderArea;
		m_clearColor = clearColor;
		m_depth = depth;
		m_stencil = stencil;
		m_clearFlags = clearFlags;
		m_hasNextPass = hasNextPass;
		m_hasPrevPass = hasPrevPass;

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// TODO: Make this configurable.
		u32 attachmentDescriptionCount = 0;
		VkAttachmentDescription attachmentDescriptions[2];

		// Color attachment
		bool doClearColor = m_clearFlags & ClearColor;

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = context->swapChain.imageFormat.format; // TODO: Make this configurable.
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
			VkAttachmentDescription depthAttachment;
			depthAttachment.format = context->device.depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = doClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.flags = 0;

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

		VK_CHECK(vkCreateRenderPass(context->device.logicalDevice, &createInfo, context->allocator, &handle));

		Logger::Info("[VULKAN_RENDER_PASS] - RenderPass successfully created");
	}

	void VulkanRenderPass::Destroy(const VulkanContext* context)
	{
		Logger::Info("[VULKAN_RENDER_PASS] - Destroying RenderPass");

		if (handle)
		{
			vkDestroyRenderPass(context->device.logicalDevice, handle, context->allocator);
			handle = nullptr;
		}
	}

	void VulkanRenderPass::Begin(VulkanCommandBuffer* commandBuffer, VkFramebuffer frameBuffer) const
	{
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = handle;
		beginInfo.framebuffer = frameBuffer;
		beginInfo.renderArea.offset.x = area.x;
		beginInfo.renderArea.offset.y = area.y;
		beginInfo.renderArea.extent.width = area.z;
		beginInfo.renderArea.extent.height = area.w;

		beginInfo.clearValueCount = 0;
		beginInfo.pClearValues = nullptr;

		VkClearValue clearValues[2];
		Memory.Zero(clearValues, sizeof(VkClearValue) * 2);

		if (m_clearFlags & ClearColor)
		{
			Memory.Copy(clearValues[beginInfo.clearValueCount].color.float32, &m_clearColor, sizeof(f32) * 4);
			beginInfo.clearValueCount++;
		}

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

	void VulkanRenderPass::End(VulkanCommandBuffer* commandBuffer) const
	{
		vkCmdEndRenderPass(commandBuffer->handle);
		commandBuffer->state = VulkanCommandBufferState::Recording;
	}
}
