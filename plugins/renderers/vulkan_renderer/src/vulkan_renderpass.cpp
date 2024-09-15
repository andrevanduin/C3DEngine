
#include "vulkan_renderpass.h"

#include <frame_data.h>
#include <logger/logger.h>
#include <platform/platform.h>
#include <random/random.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <systems/system_manager.h>

#include "vulkan_formatters.h"
#include "vulkan_utils.h"

namespace C3D
{
    bool VulkanRenderpass::Create(const RenderpassConfig& config, const VulkanContext* context)
    {
        m_name       = config.name;
        m_depth      = config.depth;
        m_stencil    = config.stencil;
        m_clearFlags = config.clearFlags;
        m_clearColor = config.clearColor;
        m_context    = context;

        // Main SubPass
        VkSubpassDescription subPass = {};
        subPass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

        DynamicArray<VkAttachmentDescription> attachmentDescriptions;
        DynamicArray<VkAttachmentDescription> colorAttachmentDescriptions;
        DynamicArray<VkAttachmentDescription> depthAttachmentDescriptions;

        // We can always just look at the first target since they are all the same (one per frame)
        for (auto& attachmentConfig : config.target.attachments)
        {
            VkAttachmentDescription attachmentDescription = {};
            if (attachmentConfig.type & RenderTargetAttachmentTypeColor)
            {
                // A color attachment
                auto doClearColor = m_clearFlags & ClearColorBuffer;

                if (attachmentConfig.source == RenderTargetAttachmentSource::Default)
                {
                    attachmentDescription.format = m_context->swapchain.imageFormat.format;
                }
                else
                {
                    // TODO: Configurable format?
                    attachmentDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
                }

                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

                // Determine which load operation to use.
                if (attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::DontCare)
                {
                    attachmentDescription.loadOp = doClearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                }
                else
                {
                    // If we are loading, check if we are also clearing. This combination doesn't make sense, so we
                    // should warn about it
                    if (attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::Load)
                    {
                        if (doClearColor)
                        {
                            WARN_LOG(
                                "Color attachment load operation is set to load, but is also set to clear. This combination is invalid and "
                                "should not be used.");
                            attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        }
                        else
                        {
                            attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }
                    }
                    else
                    {
                        FATAL_LOG("Invalid and unsupported combination of load operation ({}) and clear flags ({}) for color attachment.",
                                  attachmentDescription.loadOp, m_clearFlags);
                        return false;
                    }
                }

                // Determine which store operation to use
                if (attachmentConfig.storeOperation == RenderTargetAttachmentStoreOperation::DontCare)
                {
                    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                }
                else if (attachmentConfig.storeOperation == RenderTargetAttachmentStoreOperation::Store)
                {
                    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                }
                else
                {
                    FATAL_LOG("Invalid store operation ({}) set for attachment. Check your configuration.",
                              ToUnderlying(attachmentConfig.storeOperation));
                    return false;
                }

                // NOTE: These wil never be used for a color attachment
                attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                // If loading, that means coming from another pass, meaning the format should be
                // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL otherwise it's undefined
                attachmentDescription.initialLayout = attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::Load
                                                          ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                                          : VK_IMAGE_LAYOUT_UNDEFINED;

                // If this is the last pass writing to this attachment, present after should be set to true
                attachmentDescription.finalLayout =
                    attachmentConfig.presentAfter ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescription.flags = 0;

                // Add this attachment description to our array
                colorAttachmentDescriptions.PushBack(attachmentDescription);
            }
            else if (attachmentConfig.type & RenderTargetAttachmentTypeDepth)
            {
                // A depth attachment
                auto doClearDepth   = m_clearFlags & RenderpassClearFlag::ClearDepthBuffer;
                auto doClearStencil = m_clearFlags & RenderpassClearFlag::ClearStencilBuffer;

                auto depthFormat = m_context->device.GetDepthFormat();

                if (attachmentConfig.source == RenderTargetAttachmentSource::Default)
                {
                    attachmentDescription.format = depthFormat;
                }
                else
                {
                    // TODO: There may be a more optimal format to use when not the default depth target.
                    attachmentDescription.format = depthFormat;
                }

                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                // Determine which load operation to use
                if (attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::DontCare)
                {
                    attachmentDescription.loadOp        = doClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachmentDescription.stencilLoadOp = doClearStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                }
                else
                {
                    // If we are loading, check if we are also clearing. This combination does not make sense so we warn
                    // to user.
                    if (attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::Load)
                    {
                        // Depth
                        if (doClearDepth)
                        {
                            WARN_LOG(
                                "Depth attachment load operation set to load, but also set to clear. This combination is invalid and "
                                "should not be used.");
                            attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        }
                        else
                        {
                            attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }
                        // Stencil
                        if (doClearStencil)
                        {
                            WARN_LOG(
                                "Stencil attachment load operation set to load, but also set to clear. This combination is invalid and "
                                "should not be used.");
                            attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        }
                        else
                        {
                            attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }
                    }
                    else
                    {
                        FATAL_LOG("Invalid and unsupported combination of load operation ({}) and clear flags ({}) for depth attachment.",
                                  attachmentDescription.loadOp, m_clearFlags);
                        return false;
                    }
                }

                // Determine the store operation to use.
                if (attachmentConfig.storeOperation == RenderTargetAttachmentStoreOperation::DontCare)
                {
                    attachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                }
                else if (attachmentConfig.storeOperation == RenderTargetAttachmentStoreOperation::Store)
                {
                    attachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                }
                else
                {
                    FATAL_LOG("Invalid store operation ({}) set for depth attachment. Check your configuration.",
                              ToUnderlying(attachmentConfig.storeOperation));
                    return false;
                }

                // If coming from a previous pass, should already be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
                // Otherwise undefined.
                attachmentDescription.initialLayout = attachmentConfig.loadOperation == RenderTargetAttachmentLoadOperation::Load
                                                          ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                                          : VK_IMAGE_LAYOUT_UNDEFINED;

                attachmentDescription.finalLayout = attachmentConfig.presentAfter ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                                                  : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                depthAttachmentDescriptions.PushBack(attachmentDescription);
            }
            else
            {
                FATAL_LOG("Invalid attachmentConfig.type == '{}'", ToString(attachmentConfig.type));
            }

            // Always push to the general array
            attachmentDescriptions.PushBack(attachmentDescription);
        }

        // Setup the attachment references
        u32 attachmentsAdded = 0;

        // Color attachment reference
        VkAttachmentReference* colorAttachmentReferences = nullptr;
        auto colorAttachmentCount                        = colorAttachmentDescriptions.Size();
        if (colorAttachmentCount > 0)
        {
            colorAttachmentReferences = Memory.Allocate<VkAttachmentReference>(MemoryType::Array, colorAttachmentCount);
            for (u32 i = 0; i < colorAttachmentCount; i++)
            {
                colorAttachmentReferences[i].attachment = attachmentsAdded;
                colorAttachmentReferences[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentsAdded++;
            }

            subPass.colorAttachmentCount = static_cast<u32>(colorAttachmentCount);
            subPass.pColorAttachments    = colorAttachmentReferences;
        }
        else
        {
            subPass.colorAttachmentCount = 0;
            subPass.pColorAttachments    = nullptr;
        }

        // Depth attachment reference
        VkAttachmentReference* depthAttachmentReferences = nullptr;
        auto depthAttachmentCount                        = depthAttachmentDescriptions.Size();
        if (depthAttachmentCount > 0)
        {
            assert(depthAttachmentCount == 1);
            depthAttachmentReferences = Memory.Allocate<VkAttachmentReference>(MemoryType::Array, depthAttachmentCount);
            for (u32 i = 0; i < depthAttachmentCount; i++)
            {
                depthAttachmentReferences[i].attachment = attachmentsAdded;
                depthAttachmentReferences[i].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachmentsAdded++;
            }

            // Depth stencil data
            subPass.pDepthStencilAttachment = depthAttachmentReferences;
        }
        else
        {
            subPass.pDepthStencilAttachment = nullptr;
        }

        // Input from a Shader.
        subPass.inputAttachmentCount = 0;
        subPass.pInputAttachments    = nullptr;

        // Attachments used for multi-sampling color attachments.
        subPass.pResolveAttachments = nullptr;

        // Attachments not used in this subPass, but must be preserved for the next.
        subPass.preserveAttachmentCount = 0;
        subPass.pPreserveAttachments    = nullptr;

        // RenderPass dependencies
        VkSubpassDependency dependency;
        dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass      = 0;
        dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask   = 0;
        dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = 0;

        // RenderPass Create
        VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        createInfo.attachmentCount        = static_cast<u32>(attachmentDescriptions.Size());
        createInfo.pAttachments           = attachmentDescriptions.GetData();
        createInfo.subpassCount           = 1;
        createInfo.pSubpasses             = &subPass;
        createInfo.dependencyCount        = 1;
        createInfo.pDependencies          = &dependency;
        createInfo.pNext                  = nullptr;
        createInfo.flags                  = 0;

        VK_CHECK(vkCreateRenderPass(m_context->device.GetLogical(), &createInfo, m_context->allocator, &handle));

        auto name = String::FromFormat("VULKAN_RENDERPASS_{}", config.name);
        VulkanUtils::SetDebugObjectName(m_context, VK_OBJECT_TYPE_RENDER_PASS, handle, name);

        if (colorAttachmentReferences)
        {
            Memory.Free(colorAttachmentReferences);
        }

        if (depthAttachmentReferences)
        {
            Memory.Free(depthAttachmentReferences);
        }

        INFO_LOG("RenderPass: '{}' successfully created.", config.name);
        return true;
    }

    void VulkanRenderpass::Destroy()
    {
        INFO_LOG("Destroying RenderPass.");
        if (handle)
        {
            vkDestroyRenderPass(m_context->device.GetLogical(), handle, m_context->allocator);
            handle = nullptr;
        }
    }

    void VulkanRenderpass::Begin(VulkanCommandBuffer* commandBuffer, const Viewport* viewport, const RenderTarget& target) const
    {
        VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        beginInfo.renderPass            = handle;
        beginInfo.framebuffer           = static_cast<VkFramebuffer>(target.internalFrameBuffer);

        const auto rect = viewport->GetRect2D();

        beginInfo.renderArea.offset.x      = rect.x;
        beginInfo.renderArea.offset.y      = rect.y;
        beginInfo.renderArea.extent.width  = rect.width;
        beginInfo.renderArea.extent.height = rect.height;

        beginInfo.clearValueCount = 0;
        beginInfo.pClearValues    = nullptr;

        VkClearValue clearValues[2] = {};

        if (m_clearFlags & ClearColorBuffer)
        {
            std::memcpy(clearValues[beginInfo.clearValueCount].color.float32, &m_clearColor, sizeof(f32) * 4);
            beginInfo.clearValueCount++;
        }
        else
        {
            // If the first attachment is color, add a clear value anyway, but don't bother copying data since it will be ignored.
            // This must be done bbecause each attachment must have a clear value, even if we aren't using it.
            if (target.attachments[0].type == RenderTargetAttachmentTypeColor)
            {
                beginInfo.clearValueCount++;
            }
        }

        const bool doClearDepth   = (m_clearFlags & ClearDepthBuffer);
        const bool doClearStencil = (m_clearFlags & ClearStencilBuffer);
        if (doClearDepth || doClearStencil)
        {
            std::memcpy(clearValues[beginInfo.clearValueCount].color.float32, &m_clearColor, sizeof(f32) * 4);
            clearValues[beginInfo.clearValueCount].depthStencil.depth   = doClearDepth ? m_depth : 0;
            clearValues[beginInfo.clearValueCount].depthStencil.stencil = doClearStencil ? m_stencil : 0;
            beginInfo.clearValueCount++;
        }
        else
        {
            for (auto attachment : target.attachments)
            {
                if (attachment.type & RenderTargetAttachmentTypeDepth || attachment.type & RenderTargetAttachmentTypeStencil)
                {
                    // If there is a depth/stencil attachment, make sure to add the clear count, but don't bother copying the
                    // data
                    beginInfo.clearValueCount++;
                }
            }
        }

        beginInfo.pClearValues = beginInfo.clearValueCount > 0 ? clearValues : nullptr;

        vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        commandBuffer->state = VulkanCommandBufferState::InRenderPass;

        Renderer.BeginDebugLabel(m_name, Random.GenerateColor());
    }

    void VulkanRenderpass::End(VulkanCommandBuffer* commandBuffer) const
    {
        vkCmdEndRenderPass(commandBuffer->handle);
        commandBuffer->state = VulkanCommandBufferState::Recording;

        Renderer.EndDebugLabel();
    }
}  // namespace C3D
