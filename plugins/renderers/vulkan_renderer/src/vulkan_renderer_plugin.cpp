
#include "vulkan_renderer_plugin.h"

#include <core/engine.h>
#include <core/events/event_context.h>
#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <platform/platform.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/vertex.h>
#include <resources/loaders/text_loader.h>
#include <resources/shaders/shader.h>
#include <resources/textures/texture.h>
#include <shaderc/shaderc.h>
#include <shaderc/status.h>
#include <systems/events/event_system.h>
#include <systems/resources/resource_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include "platform/vulkan_platform.h"
#include "vulkan_allocator.h"
#include "vulkan_command_buffer.h"
#include "vulkan_debugger.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_renderpass.h"
#include "vulkan_shader.h"
#include "vulkan_swapchain.h"
#include "vulkan_utils.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "VULKAN_RENDERER";

    VulkanRendererPlugin::VulkanRendererPlugin() {}

    bool VulkanRendererPlugin::Init(const RendererPluginConfig& config, u8* outWindowRenderTargetCount)
    {
        INFO_LOG("Initializing.");

        type = RendererPluginType::Vulkan;

#ifdef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
        m_context.allocator = Memory.Allocate<VkAllocationCallbacks>(MemoryType::RenderSystem);
        if (!VulkanAllocator::Create(m_context.allocator))
        {
            ERROR_LOG("Creation of Custom Vulkan Allocator failed.");
            return false;
        }
#else
        m_context.allocator = nullptr;
#endif

        // Just set some basic default values. They will be overridden anyway.
        m_context.frameBufferWidth  = 1280;
        m_context.frameBufferHeight = 720;
        m_config                    = config;
        m_pSystemsManager           = config.pSystemsManager;

        if (!VulkanInstance::Create(m_context, config.applicationName, config.applicationVersion))
        {
            ERROR_LOG("Creation of Vulkan Instance failed.");
            return false;
        }

        // TODO: Implement multiThreading
        m_context.multiThreadingEnabled = false;

        if (!VulkanDebugger::Create(m_context))
        {
            ERROR_LOG("Create of Vulkan Debugger failed.");
            return false;
        }

        if (!VulkanPlatform::CreateSurface(m_pSystemsManager, m_context))
        {
            ERROR_LOG("Failed to create Vulkan Surface.");
            return false;
        }

        if (!m_context.device.Create(&m_context))
        {
            ERROR_LOG("Failed to create Vulkan Device.");
            return false;
        }

        m_context.swapChain.Create(m_pSystemsManager, &m_context, m_context.frameBufferWidth, m_context.frameBufferHeight, config.flags);

        // Save the number of images we have as a the number of render targets required
        *outWindowRenderTargetCount = static_cast<u8>(m_context.swapChain.imageCount);

        CreateCommandBuffers();
        INFO_LOG("Command Buffers Initialized.");

        m_context.imageAvailableSemaphores.Resize(m_context.swapChain.maxFramesInFlight);
        m_context.queueCompleteSemaphores.Resize(m_context.swapChain.maxFramesInFlight);

        INFO_LOG("Creating Semaphores and Fences.");
        auto logicalDevice = m_context.device.GetLogical();
        for (u8 i = 0; i < m_context.swapChain.maxFramesInFlight; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.imageAvailableSemaphores[i]);
            vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.queueCompleteSemaphores[i]);

            VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(logicalDevice, &fenceCreateInfo, m_context.allocator, &m_context.inFlightFences[i]));
        }

        for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
        {
            m_context.imagesInFlight[i] = nullptr;
        }

        constexpr u64 stagingBufferSize = MebiBytes(256);
        if (!m_context.stagingBuffer.Create(RenderBufferType::Staging, stagingBufferSize, RenderBufferTrackType::Linear))
        {
            ERROR_LOG("Error creating staging buffer.");
            return false;
        }
        m_context.stagingBuffer.Bind(0);

        m_context.shaderCompiler = shaderc_compiler_initialize();
        if (!m_context.shaderCompiler)
        {
            ERROR_LOG("Failed to initialize shaderc compiler.");
            return false;
        }

        INFO_LOG("Successfully Initialized.");
        return true;
    }

    void VulkanRendererPlugin::Shutdown()
    {
        INFO_LOG("Shutting down.");

        // Wait for our device to be finished with it's current frame
        m_context.device.WaitIdle();

        if (m_context.shaderCompiler)
        {
            shaderc_compiler_release(m_context.shaderCompiler);
            m_context.shaderCompiler = nullptr;
        }

        // Destroy the samplers
        for (auto sampler : m_context.samplers)
        {
            if (sampler)
            {
                WARN_LOG(
                    "Sampler is not destroyed before Shutdown is called. This indicates that you are missing a "
                    "ReleaseTextureMapResources call somewhere.");
                vkDestroySampler(m_context.device.GetLogical(), sampler, m_context.allocator);
            }
        }
        m_context.samplers.Destroy();

        m_context.stagingBuffer.Destroy();

        INFO_LOG("Destroying Semaphores and Fences.");
        auto logicalDevice = m_context.device.GetLogical();
        for (u8 i = 0; i < m_context.swapChain.maxFramesInFlight; i++)
        {
            if (m_context.imageAvailableSemaphores[i])
            {
                vkDestroySemaphore(logicalDevice, m_context.imageAvailableSemaphores[i], m_context.allocator);
                m_context.imageAvailableSemaphores[i] = nullptr;
            }
            if (m_context.queueCompleteSemaphores[i])
            {
                vkDestroySemaphore(logicalDevice, m_context.queueCompleteSemaphores[i], m_context.allocator);
                m_context.queueCompleteSemaphores[i] = nullptr;
            }
            vkDestroyFence(logicalDevice, m_context.inFlightFences[i], m_context.allocator);
        }

        m_context.imageAvailableSemaphores.Destroy();
        m_context.queueCompleteSemaphores.Destroy();

        INFO_LOG("Freeing Command buffers.");
        auto graphicsCommandPool = m_context.device.GetGraphicsCommandPool();
        for (auto& buffer : m_context.graphicsCommandBuffers)
        {
            buffer.Free(&m_context, graphicsCommandPool);
        }
        m_context.graphicsCommandBuffers.Destroy();

        INFO_LOG("Destroying SwapChain.");
        m_context.swapChain.Destroy();

        INFO_LOG("Destroying Device.");
        m_context.device.Destroy();

        INFO_LOG("Destroying Vulkan Surface.");
        if (m_context.surface)
        {
            vkDestroySurfaceKHR(m_context.instance, m_context.surface, m_context.allocator);
            m_context.surface = nullptr;
        }

        VulkanDebugger::Destroy(m_context);

        INFO_LOG("Destroying Instance.");
        vkDestroyInstance(m_context.instance, m_context.allocator);

#if C3D_VULKAN_USE_CUSTOM_ALLOCATOR == 1
        if (m_context.allocator)
        {
            Memory.Free(m_context.allocator);
            m_context.allocator = nullptr;
        }
#endif

        INFO_LOG("Complete.");
    }

    void VulkanRendererPlugin::OnResize(const u32 width, const u32 height)
    {
        m_context.frameBufferWidth  = width;
        m_context.frameBufferHeight = height;
        m_context.frameBufferSizeGeneration++;

        INFO_LOG("Width: {}, Height: {} and Generation: {}.", width, height, m_context.frameBufferSizeGeneration);
    }

    bool VulkanRendererPlugin::PrepareFrame(const FrameData& frameData)
    {
        const auto& device = m_context.device;

        // If we are recreating the SwapChain we should stop this frame
        if (m_context.recreatingSwapChain)
        {
            const auto result = device.WaitIdle();
            if (!VulkanUtils::IsSuccess(result))
            {
                ERROR_LOG("vkDeviceWaitIdle (1) failed: '{}'.", VulkanUtils::ResultString(result, true));
                return false;
            }
            INFO_LOG("Recreating SwapChain. Stopping PrepareFrame().");
            return false;
        }

        // If the FrameBuffer was resized or a render flag was changed we must also create a new SwapChain.
        if (m_context.frameBufferSizeGeneration != m_context.frameBufferSizeLastGeneration || m_context.renderFlagChanged)
        {
            // FrameBuffer was resized. We need to recreate it.
            const auto result = device.WaitIdle();
            if (!VulkanUtils::IsSuccess(result))
            {
                ERROR_LOG("vkDeviceWaitIdle (2) failed: '{}'.", VulkanUtils::ResultString(result, true));
                return false;
            }

            if (!RecreateSwapChain())
            {
                return false;
            }

            // Reset our render flag changed flag
            m_context.renderFlagChanged = false;

            INFO_LOG("SwapChain Resized successfully. Stopping PrepareFrame().");
            return false;
        }

        // Reset our staging buffer for the next frame
        if (!m_context.stagingBuffer.Clear(false))
        {
            ERROR_LOG("Failed to clear staging buffer.");
            return false;
        }

        // Wait for the execution of the current frame to complete.
        const VkResult result =
            vkWaitForFences(device.GetLogical(), 1, &m_context.inFlightFences[m_context.currentFrame], true, UINT64_MAX);
        if (!VulkanUtils::IsSuccess(result))
        {
            FATAL_LOG("vkWaitForFences() failed: '{}'.", VulkanUtils::ResultString(result));
            return false;
        }

        // Acquire the next image from the SwapChain. Pass along the semaphore that should be signaled when this
        // completes. This same semaphore will later be waited on by the queue submission to ensure this image is
        // available.
        if (!m_context.swapChain.AcquireNextImageIndex(UINT64_MAX, m_context.imageAvailableSemaphores[m_context.currentFrame], nullptr,
                                                       &m_context.imageIndex))
        {
            ERROR_LOG("Failed to acquire next image index.");
            return false;
        }

        // Reset fences for next frame
        VK_CHECK(vkResetFences(m_context.device.GetLogical(), 1, &m_context.inFlightFences[m_context.currentFrame]));

        return true;
    }

    bool VulkanRendererPlugin::Begin(const FrameData& frameData)
    {
        // We can begin recording commands
        const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

        commandBuffer->Reset();
        commandBuffer->Begin(false, false, false);

        // Always start each frame with Counter-Clockwise winding
        SetWinding(RendererWinding::CounterClockwise);

        // Reset stencil reference
        SetStencilReference(0);
        // Reset compare mask
        SetStencilCompareMask(0xFF);
        // Reset stencil operation
        SetStencilOperation(StencilOperation::Keep, StencilOperation::Replace, StencilOperation::Keep, CompareOperation::Always);
        // Disable stencil testing by default
        SetStencilTestingEnabled(false);
        // Enable depth testing by default
        SetDepthTestingEnabled(true);
        // Disable stencil writing by default
        SetStencilWriteMask(0x0);

        return true;
    }

    bool VulkanRendererPlugin::End(const FrameData& frameData)
    {
        const auto commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];

        // End the CommandBuffer
        commandBuffer->End();

        VkSubmitInfo submitInfo       = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer->handle;

        // The semaphores to be signaled when the queue is complete
        if (drawIndex == 0)
        {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores    = &m_context.queueCompleteSemaphores[m_context.currentFrame];

            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores    = &m_context.imageAvailableSemaphores[m_context.currentFrame];
        }
        else
        {
            submitInfo.signalSemaphoreCount = 0;

            submitInfo.waitSemaphoreCount = 0;
        }

        // Each semaphore waits on the corresponding pipeline stage to complete at a 1:1 ratio.
        // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent color attachment writes from executing
        // unitil the semaphore signals (ensuring that only one frame is presented at a time).
        constexpr VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.pWaitDstStageMask            = flags;

        // Submit all the commands that we have queued
        auto graphicsQueue = m_context.device.GetGraphicsQueue();
        const auto result  = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_context.inFlightFences[m_context.currentFrame]);

        if (result != VK_SUCCESS)
        {
            ERROR_LOG("vkQueueSubmit failed with result: '{}'.", VulkanUtils::ResultString(result, true));
            return false;
        }

        // Queue submission is done
        commandBuffer->UpdateSubmitted();

        // For timing purposes, wait for the queue to complete.
        // This gives an accurate picture of how long the render takes, including the work submitted to the actual queue.
        vkWaitForFences(m_context.device.GetLogical(), 1, &m_context.inFlightFences[m_context.currentFrame], true, UINT64_MAX);

        return true;
    }

    bool VulkanRendererPlugin::Present(const FrameData& frameData)
    {
        // Present the image (and give it back to the SwapChain)
        auto presentQueue = m_context.device.GetPresentQueue();
        m_context.swapChain.Present(presentQueue, m_context.queueCompleteSemaphores[m_context.currentFrame], m_context.imageIndex);

        return true;
    }

    void VulkanRendererPlugin::SetViewport(const vec4& rect)
    {
        VkViewport viewport;
        viewport.x        = rect.x;
        viewport.y        = rect.y;
        viewport.width    = rect.z;
        viewport.height   = rect.w;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        const auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        vkCmdSetViewport(commandBuffer.handle, 0, 1, &viewport);
    }

    void VulkanRendererPlugin::ResetViewport()
    {
        // Just set viewport to our currently stored rect
        SetViewport(m_context.viewportRect);
    }

    void VulkanRendererPlugin::SetScissor(const ivec4& rect)
    {
        VkRect2D scissor;
        scissor.offset.x      = rect.x;
        scissor.offset.y      = rect.y;
        scissor.extent.width  = rect.z;
        scissor.extent.height = rect.w;

        const auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        vkCmdSetScissor(commandBuffer.handle, 0, 1, &scissor);
    }

    void VulkanRendererPlugin::ResetScissor() { SetScissor(m_context.scissorRect); }

    void VulkanRendererPlugin::SetWinding(const RendererWinding winding)
    {
        const auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];

        VkFrontFace frontFace = winding == RendererWinding::CounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;

        // Check for Dynamic Winding
        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE))
        {
            // Native support
            vkCmdSetFrontFace(commandBuffer.handle, frontFace);
        }
        else if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            // Support by means of extension
            m_context.pfnCmdSetFrontFaceEXT(commandBuffer.handle, frontFace);
        }
        else
        {
            // No support (so we fallback to binding a different pipeline)
            if (m_context.boundShader)
            {
                auto vulkanShader = static_cast<VulkanShader*>(m_context.boundShader->apiSpecificData);
                if (winding == RendererWinding::CounterClockwise)
                {
                    vulkanShader->pipelines[vulkanShader->boundPipeline]->Bind(&commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                }
                else
                {
                    vulkanShader->clockwisePipelines[vulkanShader->boundPipeline]->Bind(&commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                }
            }
            else
            {
                ERROR_LOG("Unable to set Winding since there is no currently bound shader.");
            }
        }
    }

    static VkStencilOp GetStencilOp(StencilOperation op)
    {
        switch (op)
        {
            case StencilOperation::Keep:
                return VK_STENCIL_OP_KEEP;
            case StencilOperation::Zero:
                return VK_STENCIL_OP_ZERO;
            case StencilOperation::Replace:
                return VK_STENCIL_OP_REPLACE;
            case StencilOperation::IncrementAndClamp:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case StencilOperation::DecrementAndClamp:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case StencilOperation::Invert:
                return VK_STENCIL_OP_INVERT;
            case StencilOperation::IncrementAndWrap:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case StencilOperation::DecrementAndWrap:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            default:
                ERROR_LOG("Unsupported StencilOperation. Defaulting to KEEP.");
                return VK_STENCIL_OP_KEEP;
        }
    }

    static VkCompareOp GetCompareOp(CompareOperation op)
    {
        switch (op)
        {
            case CompareOperation::Never:
                return VK_COMPARE_OP_NEVER;
            case CompareOperation::Less:
                return VK_COMPARE_OP_LESS;
            case CompareOperation::Equal:
                return VK_COMPARE_OP_EQUAL;
            case CompareOperation::LessOrEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOperation::Greater:
                return VK_COMPARE_OP_GREATER;
            case CompareOperation::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOperation::GreaterOrEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOperation::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                ERROR_LOG("Unsupported CompareOperation. Defaulting to ALWAYS.");
                return VK_COMPARE_OP_ALWAYS;
        }
    }

    void VulkanRendererPlugin::SetStencilTestingEnabled(bool enabled)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];

        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE))
        {
            vkCmdSetStencilTestEnable(commandBuffer.handle, static_cast<VkBool32>(enabled));
        }
        else if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            m_context.pfnCmdSetStencilTestEnableEXT(commandBuffer.handle, static_cast<VkBool32>(enabled));
        }
        else
        {
            FATAL_LOG("Unsupported functionality.");
        }
    }
    void VulkanRendererPlugin::SetStencilReference(u32 reference)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        // Supported since VK_VERSION_1_0 so no need for fallback to extension
        vkCmdSetStencilReference(commandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
    }

    void VulkanRendererPlugin::SetStencilCompareMask(u32 compareMask)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        // Supported since VK_VERSION_1_0 so no need for fallback to extension
        vkCmdSetStencilCompareMask(commandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, compareMask);
    }

    void VulkanRendererPlugin::SetStencilWriteMask(u32 writeMask)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        // Supported since VK_VERSION_1_0 so no need for fallback to extension
        vkCmdSetStencilWriteMask(commandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, writeMask);
    }

    void VulkanRendererPlugin::SetStencilOperation(StencilOperation failOp, StencilOperation passOp, StencilOperation depthFailOp,
                                                   CompareOperation compareOp)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];

        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE))
        {
            vkCmdSetStencilOp(commandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, GetStencilOp(failOp), GetStencilOp(passOp),
                              GetStencilOp(depthFailOp), GetCompareOp(compareOp));
        }
        else if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            m_context.pfnCmdSetStencilOpEXT(commandBuffer.handle, VK_STENCIL_FACE_FRONT_AND_BACK, GetStencilOp(failOp),
                                            GetStencilOp(passOp), GetStencilOp(depthFailOp), GetCompareOp(compareOp));
        }
        else
        {
            FATAL_LOG("Unsupported functionality.");
        }
    }

    void VulkanRendererPlugin::SetDepthTestingEnabled(bool enabled)
    {
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];

        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE))
        {
            vkCmdSetDepthTestEnable(commandBuffer.handle, static_cast<VkBool32>(enabled));
        }
        else if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            m_context.pfnCmdSetDepthTestEnableEXT(commandBuffer.handle, static_cast<VkBool32>(enabled));
        }
        else
        {
            FATAL_LOG("Unsupported functionality.");
        }
    }

    void VulkanRendererPlugin::CreateRenderTarget(RenderPass* pass, RenderTarget& target, u32 width, u32 height)
    {
        VkImageView attachmentViews[32] = { 0 };
        for (u32 i = 0; i < target.attachments.Size(); i++)
        {
            attachmentViews[i] = static_cast<VulkanImage*>(target.attachments[i].texture->internalData)->view;
        }

        // Setup our frameBuffer creation
        VkFramebufferCreateInfo frameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        frameBufferCreateInfo.renderPass              = dynamic_cast<VulkanRenderPass*>(pass)->handle;
        frameBufferCreateInfo.attachmentCount         = target.attachments.Size();
        frameBufferCreateInfo.pAttachments            = attachmentViews;
        frameBufferCreateInfo.width                   = width;
        frameBufferCreateInfo.height                  = height;
        frameBufferCreateInfo.layers                  = 1;

        VK_CHECK(vkCreateFramebuffer(m_context.device.GetLogical(), &frameBufferCreateInfo, m_context.allocator,
                                     reinterpret_cast<VkFramebuffer*>(&target.internalFrameBuffer)));
    }

    void VulkanRendererPlugin::DestroyRenderTarget(RenderTarget& target, bool freeInternalMemory)
    {
        if (target.internalFrameBuffer)
        {
            vkDestroyFramebuffer(m_context.device.GetLogical(), static_cast<VkFramebuffer>(target.internalFrameBuffer),
                                 m_context.allocator);
            target.internalFrameBuffer = nullptr;

            if (freeInternalMemory)
            {
                target.attachments.Destroy();
            }
        }
    }

    RenderPass* VulkanRendererPlugin::CreateRenderPass(const RenderPassConfig& config)
    {
        const auto pass = Memory.New<VulkanRenderPass>(MemoryType::RenderSystem, m_pSystemsManager, &m_context, config);
        if (!pass->Create(config))
        {
            ERROR_LOG("Failed to create RenderPass: '{}'.", config.name);
            return nullptr;
        }
        return pass;
    }

    bool VulkanRendererPlugin::DestroyRenderPass(RenderPass* pass)
    {
        const auto vulkanPass = dynamic_cast<VulkanRenderPass*>(pass);
        vulkanPass->Destroy();
        Memory.Delete(pass);
        return true;
    }

    RenderBuffer* VulkanRendererPlugin::CreateRenderBuffer(const String& name, const RenderBufferType bufferType, const u64 totalSize,
                                                           RenderBufferTrackType trackType)
    {
        const auto buffer = Memory.New<VulkanBuffer>(MemoryType::RenderSystem, &m_context, name);
        if (!buffer->Create(bufferType, totalSize, trackType)) return nullptr;
        return buffer;
    }

    bool VulkanRendererPlugin::DestroyRenderBuffer(RenderBuffer* buffer)
    {
        buffer->Destroy();
        Memory.Delete(buffer);
        return true;
    }

    Texture* VulkanRendererPlugin::GetWindowAttachment(const u8 index)
    {
        if (index >= m_context.swapChain.imageCount)
        {
            FATAL_LOG("Attempting to get attachment index that is out of range: '{}'. Attachment count is: '{}'.", index,
                      m_context.swapChain.imageCount);
            return nullptr;
        }
        return &m_context.swapChain.renderTextures[index];
    }

    Texture* VulkanRendererPlugin::GetDepthAttachment(u8 index)
    {
        if (index >= m_context.swapChain.imageCount)
        {
            FATAL_LOG("Attempting to get attachment index that is out of range: '{}'. Attachment count is: '{}'.", index,
                      m_context.swapChain.imageCount);
            return nullptr;
        }
        return &m_context.swapChain.depthTextures[index];
    }

    u8 VulkanRendererPlugin::GetWindowAttachmentIndex() { return static_cast<u8>(m_context.imageIndex); }

    u8 VulkanRendererPlugin::GetWindowAttachmentCount() { return static_cast<u8>(m_context.swapChain.imageCount); }

    bool VulkanRendererPlugin::IsMultiThreaded() const { return m_context.multiThreadingEnabled; }

    void VulkanRendererPlugin::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled)
    {
        m_config.flags              = enabled ? (m_config.flags | flag) : (m_config.flags & ~flag);
        m_context.renderFlagChanged = true;
    }

    bool VulkanRendererPlugin::IsFlagEnabled(const RendererConfigFlagBits flag) const { return m_config.flags & flag; }

    bool VulkanRendererPlugin::BeginRenderPass(RenderPass* pass, const C3D::FrameData& frameData)
    {
        VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
        const auto vulkanPass              = dynamic_cast<VulkanRenderPass*>(pass);
        vulkanPass->Begin(commandBuffer, frameData);
        return true;
    }

    bool VulkanRendererPlugin::EndRenderPass(RenderPass* pass)
    {
        VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
        const auto vulkanPass              = dynamic_cast<VulkanRenderPass*>(pass);
        vulkanPass->End(commandBuffer);
        return true;
    }

    void VulkanRendererPlugin::CreateTexture(const u8* pixels, Texture* texture)
    {
        // Internal data creation
        texture->internalData = Memory.New<VulkanImage>(MemoryType::Texture);

        const auto image             = static_cast<VulkanImage*>(texture->internalData);
        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture->width) * texture->height * texture->channelCount *
                                       (texture->type == TextureType::TypeCube ? 6 : 1);

        // NOTE: Assumes 8 bits per channel
        constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        image->Create(&m_context, texture->name, texture->type, texture->width, texture->height, imageFormat, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture->mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);

        // Load the data
        WriteDataToTexture(texture, 0, static_cast<u32>(imageSize), pixels);
        texture->generation++;
    }

    VkFormat ChannelCountToFormat(const u8 channelCount, const VkFormat defaultFormat)
    {
        switch (channelCount)
        {
            case 1:
                return VK_FORMAT_R8_UNORM;
            case 2:
                return VK_FORMAT_R8G8_UNORM;
            case 3:
                return VK_FORMAT_R8G8B8_UNORM;
            case 4:
                return VK_FORMAT_R8G8B8A8_UNORM;
            default:
                return defaultFormat;
        }
    }

    void VulkanRendererPlugin::CreateWritableTexture(Texture* texture)
    {
        texture->internalData = Memory.New<VulkanImage>(MemoryType::Texture);
        const auto image      = static_cast<VulkanImage*>(texture->internalData);

        VkFormat imageFormat;
        VkImageUsageFlagBits usage;
        VkImageAspectFlagBits aspect;

        if (texture->flags & TextureFlag::IsDepth)
        {
            usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            aspect      = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageFormat = m_context.device.GetDepthFormat();
        }
        else
        {
            usage       = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            aspect      = VK_IMAGE_ASPECT_COLOR_BIT;
            imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);
        }

        image->Create(&m_context, texture->name, texture->type, texture->width, texture->height, imageFormat, VK_IMAGE_TILING_OPTIMAL,
                      usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture->mipLevels, aspect);

        texture->generation++;
    }

    void VulkanRendererPlugin::WriteDataToTexture(Texture* texture, u32 offset, const u32 size, const u8* pixels)
    {
        const auto image           = static_cast<VulkanImage*>(texture->internalData);
        const VkFormat imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // Allocate space in our staging buffer
        u64 stagingOffset = 0;
        if (!m_context.stagingBuffer.Allocate(size, stagingOffset))
        {
            ERROR_LOG("Failed to allocate in the staging buffer.");
            return;
        }

        // Load the data into our staging buffer
        if (!m_context.stagingBuffer.LoadRange(stagingOffset, size, pixels))
        {
            ERROR_LOG("Failed to load range into staging buffer.");
            return;
        }

        VulkanCommandBuffer tempCommandBuffer;
        VkCommandPool pool = m_context.device.GetGraphicsCommandPool();
        VkQueue queue      = m_context.device.GetGraphicsQueue();

        tempCommandBuffer.AllocateAndBeginSingleUse(&m_context, pool);

        // Transition the layout from whatever it is currently to optimal for receiving data.
        image->TransitionLayout(&tempCommandBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy the data from the buffer.
        image->CopyFromBuffer(texture->type, m_context.stagingBuffer.handle, stagingOffset, &tempCommandBuffer);

        if (texture->mipLevels <= 1 || !image->CreateMipMaps(&tempCommandBuffer))
        {
            // If we don't need mips or the generation of the mips fails we fallback to ordinary transition.
            // Transition from optimal for receiving data to shader-read-only optimal layout.
            image->TransitionLayout(&tempCommandBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        tempCommandBuffer.EndSingleUse(&m_context, pool, queue);

        texture->generation++;
    }

    void VulkanRendererPlugin::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight)
    {
        if (texture && texture->internalData)
        {
            const auto image = static_cast<VulkanImage*>(texture->internalData);
            Memory.Delete(image);

            const VkFormat imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);

            // Recalculate our mip levels
            if (texture->mipLevels > 1)
            {
                // Take the base-2 log from the largest dimension floor it and add 1 for the base mip level.
                texture->mipLevels = Floor(Log2(Max(newWidth, newHeight))) + 1;
            }

            // TODO: Lot's of assumptions here
            image->Create(&m_context, texture->name, texture->type, newWidth, newHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture->mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);

            texture->generation++;
        }
    }

    void VulkanRendererPlugin::ReadDataFromTexture(Texture* texture, const u32 offset, const u32 size, void** outMemory)
    {
        const auto image       = static_cast<VulkanImage*>(texture->internalData);
        const auto imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // TODO: Add a global read buffer (with freelist) which is similar to staging buffer but meant for reading
        // Create a staging buffer and load data into it
        VulkanBuffer staging(&m_context, "TEXTURE_READ_STAGING");
        if (!staging.Create(RenderBufferType::Read, size, RenderBufferTrackType::Linear))
        {
            ERROR_LOG("Failed to create staging buffer.");
            return;
        }

        staging.Bind(0);

        VulkanCommandBuffer tempBuffer;
        VkCommandPool pool = m_context.device.GetGraphicsCommandPool();
        VkQueue queue      = m_context.device.GetGraphicsQueue();

        tempBuffer.AllocateAndBeginSingleUse(&m_context, pool);

        // Transition the layout from whatever it is currently to optimal for handing out data
        image->TransitionLayout(&tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy the data to the buffer
        image->CopyToBuffer(texture->type, staging.handle, &tempBuffer);

        // Transition from optimal for data reading to shader-read-only optimal layout
        image->TransitionLayout(&tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tempBuffer.EndSingleUse(&m_context, pool, queue);

        if (!staging.Read(offset, size, outMemory))
        {
            ERROR_LOG("Failed to read from staging buffer.");
        }

        staging.Unbind();
        staging.Destroy();
    }

    void VulkanRendererPlugin::ReadPixelFromTexture(Texture* texture, const u32 x, const u32 y, u8** outRgba)
    {
        const auto image       = static_cast<VulkanImage*>(texture->internalData);
        const auto imageFormat = ChannelCountToFormat(texture->channelCount, VK_FORMAT_R8G8B8A8_UNORM);
        // RGBA is 4 * sizeof a unsigned 8bit integer
        constexpr auto size = sizeof(u8) * 4;

        // Create a staging buffer and load data into it
        VulkanBuffer staging(&m_context, "READ_PIXEL_STAGING");
        if (!staging.Create(RenderBufferType::Read, size, RenderBufferTrackType::Linear))
        {
            ERROR_LOG("Failed to create staging buffer.");
            return;
        }

        staging.Bind(0);

        VulkanCommandBuffer tempBuffer;
        VkCommandPool pool = m_context.device.GetGraphicsCommandPool();
        VkQueue queue      = m_context.device.GetGraphicsQueue();

        tempBuffer.AllocateAndBeginSingleUse(&m_context, pool);

        // Transition the layout from whatever it is currently to optimal for handing out data
        image->TransitionLayout(&tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy the data to the buffer
        image->CopyPixelToBuffer(texture->type, staging.handle, x, y, &tempBuffer);

        // Transition from optimal for data reading to shader-read-only optimal layout
        image->TransitionLayout(&tempBuffer, texture->type, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tempBuffer.EndSingleUse(&m_context, pool, queue);

        if (!staging.Read(0, size, reinterpret_cast<void**>(outRgba)))
        {
            ERROR_LOG("Failed to read from staging buffer.");
        }

        staging.Unbind();
        staging.Destroy();
    }

    void VulkanRendererPlugin::DestroyTexture(Texture* texture)
    {
        m_context.device.WaitIdle();

        if (const auto image = static_cast<VulkanImage*>(texture->internalData))
        {
            image->Destroy();
            Memory.Delete(texture->internalData);
            texture->internalData = nullptr;
        }
    }

    bool VulkanRendererPlugin::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
    {
        // Allocate enough memory for the
        shader->apiSpecificData = Memory.New<VulkanShader>(MemoryType::Shader, &m_context);

        // Translate stages
        VkShaderStageFlags vulkanStages[VULKAN_SHADER_MAX_STAGES]{};
        for (u32 i = 0; i < config.stages.Size(); i++)
        {
            switch (config.stages[i])
            {
                case ShaderStage::Fragment:
                    vulkanStages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                case ShaderStage::Vertex:
                    vulkanStages[i] = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case ShaderStage::Geometry:
                    WARN_LOG("VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.");
                    vulkanStages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
                    break;
                case ShaderStage::Compute:
                    WARN_LOG("SHADER_STAGE_COMPUTE is set but not yet supported.");
                    vulkanStages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
                    break;
            }
        }

        // TODO: Make the max descriptor allocate count configurable
        constexpr u32 maxDescriptorAllocateCount = 1024;

        // Get a pointer to our Vulkan specific shader stuff
        const auto vulkanShader                    = static_cast<VulkanShader*>(shader->apiSpecificData);
        vulkanShader->renderPass                   = dynamic_cast<VulkanRenderPass*>(pass);
        vulkanShader->config.maxDescriptorSetCount = maxDescriptorAllocateCount;

        vulkanShader->config.stageCount = 0;
        for (u32 i = 0; i < config.stageFileNames.Size(); i++)
        {
            // Make sure we have enough room left for this stage
            if (vulkanShader->config.stageCount + 1 > VULKAN_SHADER_MAX_STAGES)
            {
                ERROR_LOG("Shaders may have a maximum of {} stages.", VULKAN_SHADER_MAX_STAGES);
                return false;
            }

            // Check if we support this stage
            VkShaderStageFlagBits stageFlag;
            switch (config.stages[i])
            {
                case ShaderStage::Vertex:
                    stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case ShaderStage::Fragment:
                    stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                default:
                    ERROR_LOG("Unsupported shader stage {}. Stage ignored.", ToUnderlying(config.stages[i]));
                    continue;
            }

            // Set the stage and increment the stage count
            const auto stageIndex                            = vulkanShader->config.stageCount;
            vulkanShader->config.stages[stageIndex].stage    = stageFlag;
            vulkanShader->config.stages[stageIndex].fileName = config.stageFileNames[i].Data();
            vulkanShader->config.stageCount++;
        }

        // Zero out arrays and counts
        std::memset(vulkanShader->config.descriptorSets, 0, sizeof(VulkanDescriptorSetConfig) * 2);
        vulkanShader->config.descriptorSets[0].samplerBindingIndex = INVALID_ID_U8;
        vulkanShader->config.descriptorSets[1].samplerBindingIndex = INVALID_ID_U8;

        // Zero out attribute arrays
        std::memset(vulkanShader->config.attributes, 0, sizeof(VkVertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);

        // Get the uniform counts
        vulkanShader->ZeroOutCounts();
        for (const auto& uniform : config.uniforms)
        {
            switch (uniform.scope)
            {
                case ShaderScope::Global:
                    if (uniform.type == ShaderUniformType::Uniform_Sampler)
                        vulkanShader->globalUniformSamplerCount++;
                    else
                        vulkanShader->globalUniformCount++;
                    break;
                case ShaderScope::Instance:
                    if (uniform.type == ShaderUniformType::Uniform_Sampler)
                        vulkanShader->instanceUniformSamplerCount++;
                    else
                        vulkanShader->instanceUniformCount++;
                    break;
                case ShaderScope::Local:
                    vulkanShader->localUniformCount++;
                    break;
                case ShaderScope::None:
                    break;
            }
        }

        // TODO: For now, shaders will only ever have these 2 types of descriptor pools
        // HACK: max number of ubo descriptor sets
        vulkanShader->config.poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 };
        // HACK: max number of image sampler descriptor sets
        vulkanShader->config.poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 };

        // Global descriptor set config
        if (vulkanShader->globalUniformCount > 0 || vulkanShader->globalUniformSamplerCount > 0)
        {
            auto& setConfig = vulkanShader->config.descriptorSets[vulkanShader->config.descriptorSetCount];

            // Global UBO binding is first, if present
            if (vulkanShader->globalUniformCount > 0)
            {
                u8 bindingIndex                                  = setConfig.bindingCount;
                setConfig.bindings[bindingIndex].binding         = bindingIndex;
                setConfig.bindings[bindingIndex].descriptorCount = 1;
                setConfig.bindings[bindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                setConfig.bindings[bindingIndex].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                setConfig.bindingCount++;
            }

            // Add a binding for samplers if we are using them
            if (vulkanShader->globalUniformSamplerCount > 0)
            {
                u8 bindingIndex                                  = setConfig.bindingCount;
                setConfig.bindings[bindingIndex].binding         = bindingIndex;
                setConfig.bindings[bindingIndex].descriptorCount = vulkanShader->globalUniformSamplerCount;
                setConfig.bindings[bindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                setConfig.bindings[bindingIndex].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                setConfig.samplerBindingIndex                    = bindingIndex;
                setConfig.bindingCount++;
            }

            // Increment our descriptor set counter
            vulkanShader->config.descriptorSetCount++;
        }

        // If using instance uniforms, add a UBO descriptor set
        if (vulkanShader->instanceUniformCount > 0 || vulkanShader->instanceUniformSamplerCount > 0)
        {
            auto& setConfig = vulkanShader->config.descriptorSets[vulkanShader->config.descriptorSetCount];

            // Add a binding for UBO if it's used
            if (vulkanShader->instanceUniformCount > 0)
            {
                u8 bindingIndex                                  = setConfig.bindingCount;
                setConfig.bindings[bindingIndex].binding         = bindingIndex;
                setConfig.bindings[bindingIndex].descriptorCount = 1;
                setConfig.bindings[bindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                setConfig.bindings[bindingIndex].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                setConfig.bindingCount++;
            }

            // Add a binding for samplers if used
            if (vulkanShader->instanceUniformSamplerCount > 0)
            {
                u8 bindingIndex                                  = setConfig.bindingCount;
                setConfig.bindings[bindingIndex].binding         = bindingIndex;
                setConfig.bindings[bindingIndex].descriptorCount = vulkanShader->instanceUniformSamplerCount;
                setConfig.bindings[bindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                setConfig.bindings[bindingIndex].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                setConfig.samplerBindingIndex                    = bindingIndex;
                setConfig.bindingCount++;
            }

            // Increment our descriptor set counter
            vulkanShader->config.descriptorSetCount++;
        }

        // Invalidate all instance states
        // TODO: make this dynamic
        for (auto& instanceState : vulkanShader->instanceStates)
        {
            instanceState.id = INVALID_ID;
        }

        // Copy over our cull mode
        vulkanShader->config.cullMode      = config.cullMode;
        vulkanShader->config.topologyTypes = config.topologyTypes;

        return true;
    }

    void VulkanRendererPlugin::DestroyShader(Shader& shader)
    {
        // Make sure there is something to destroy
        if (shader.apiSpecificData)
        {
            const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);

            VkDevice logicalDevice                   = m_context.device.GetLogical();
            const VkAllocationCallbacks* vkAllocator = m_context.allocator;

            // Cleanup the descriptor set layouts
            for (u32 i = 0; i < vulkanShader->config.descriptorSetCount; i++)
            {
                if (vulkanShader->descriptorSetLayouts[i])
                {
                    vkDestroyDescriptorSetLayout(logicalDevice, vulkanShader->descriptorSetLayouts[i], vkAllocator);
                    vulkanShader->descriptorSetLayouts[i] = nullptr;
                }
            }

            // Cleanup descriptor pool
            if (vulkanShader->descriptorPool)
            {
                vkDestroyDescriptorPool(logicalDevice, vulkanShader->descriptorPool, vkAllocator);
            }

            // Cleanup Uniform buffer
            vulkanShader->uniformBuffer.UnMapMemory(0, VK_WHOLE_SIZE);
            vulkanShader->mappedUniformBufferBlock = nullptr;
            vulkanShader->uniformBuffer.Destroy();

            // Cleanup Pipelines
            for (const auto pipeline : vulkanShader->pipelines)
            {
                if (pipeline)
                {
                    pipeline->Destroy();
                    Memory.Delete(pipeline);
                }
            }
            // Do the same for our Clockwise Pipelines
            for (const auto pipeline : vulkanShader->clockwisePipelines)
            {
                if (pipeline)
                {
                    pipeline->Destroy();
                    Memory.Delete(pipeline);
                }
            }
            vulkanShader->pipelines.Destroy();
            vulkanShader->clockwisePipelines.Destroy();

            // Cleanup Shader Modules
            for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
            {
                vkDestroyShaderModule(logicalDevice, vulkanShader->stages[i].handle, vkAllocator);
            }

            // Destroy the configuration
            for (auto& stage : vulkanShader->config.stages)
            {
                stage.fileName.Destroy();
            }

            // Free the api (Vulkan in this case) specific data from the shader
            Memory.Free(shader.apiSpecificData);
            shader.apiSpecificData = nullptr;
        }
    }

    VkPrimitiveTopology GetVkPrimitiveTopology(const ShaderTopology topology)
    {
        switch (topology)
        {
            case ShaderTopology::Triangles:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case ShaderTopology::Lines:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case ShaderTopology::Points:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    bool VulkanRendererPlugin::InitializeShader(Shader& shader)
    {
        VkDevice logicalDevice                   = m_context.device.GetLogical();
        const VkAllocationCallbacks* vkAllocator = m_context.allocator;
        const auto vulkanShader                  = static_cast<VulkanShader*>(shader.apiSpecificData);

        for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
        {
            if (!CreateShaderModule(vulkanShader->config.stages[i], &vulkanShader->stages[i]))
            {
                ERROR_LOG("Unable to create '{}' shader module for '{}'. Shader will be destroyed.",
                          vulkanShader->config.stages[i].fileName, shader.name);
                return false;
            }
        }

        // Static lookup table for our types -> Vulkan ones.
        static VkFormat* types = nullptr;
        static VkFormat t[12];
        if (!types)
        {
            t[Attribute_Float32]   = VK_FORMAT_R32_SFLOAT;
            t[Attribute_Float32_2] = VK_FORMAT_R32G32_SFLOAT;
            t[Attribute_Float32_3] = VK_FORMAT_R32G32B32_SFLOAT;
            t[Attribute_Float32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
            t[Attribute_Int8]      = VK_FORMAT_R8_SINT;
            t[Attribute_UInt8]     = VK_FORMAT_R8_UINT;
            t[Attribute_Int16]     = VK_FORMAT_R16_SINT;
            t[Attribute_UInt16]    = VK_FORMAT_R16_UINT;
            t[Attribute_Int32]     = VK_FORMAT_R32_SINT;
            t[Attribute_UInt32]    = VK_FORMAT_R32_UINT;
            types                  = t;
        }

        // Process attributes
        const u64 attributeCount = shader.attributes.Size();
        u32 offset               = 0;
        for (u32 i = 0; i < attributeCount; i++)
        {
            // Setup the new attribute
            VkVertexInputAttributeDescription attribute{};
            attribute.location = i;
            attribute.binding  = 0;
            attribute.offset   = offset;
            attribute.format   = types[shader.attributes[i].type];

            vulkanShader->config.attributes[i] = attribute;
            offset += shader.attributes[i].size;
        }

        // Create descriptor pool
        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount              = 2;
        poolInfo.pPoolSizes                 = vulkanShader->config.poolSizes;
        poolInfo.maxSets                    = vulkanShader->config.maxDescriptorSetCount;
        poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &vulkanShader->descriptorPool);
        if (!VulkanUtils::IsSuccess(result))
        {
            ERROR_LOG("Failed to create descriptor pool: '{}'.", VulkanUtils::ResultString(result));
            return false;
        }

        // Create descriptor set layouts
        for (u32 i = 0; i < vulkanShader->config.descriptorSetCount; i++)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            layoutInfo.bindingCount                    = vulkanShader->config.descriptorSets[i].bindingCount;
            layoutInfo.pBindings                       = vulkanShader->config.descriptorSets[i].bindings;
            result = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, vkAllocator, &vulkanShader->descriptorSetLayouts[i]);
            if (!VulkanUtils::IsSuccess(result))
            {
                ERROR_LOG("Failed to create Descriptor Set Layout: '{}'.", VulkanUtils::ResultString(result));
                return false;
            }
        }

        // TODO: This shouldn't be here :(
        const auto fWidth  = static_cast<f32>(m_context.frameBufferWidth);
        const auto fHeight = static_cast<f32>(m_context.frameBufferHeight);

        // Viewport
        VkViewport viewport;
        viewport.x        = 0.0f;
        viewport.y        = fHeight;
        viewport.width    = fWidth;
        viewport.height   = -fHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor;
        scissor.offset.x = scissor.offset.y = 0;
        scissor.extent.width                = m_context.frameBufferWidth;
        scissor.extent.height               = m_context.frameBufferHeight;

        VkPipelineShaderStageCreateInfo stageCreateInfos[VULKAN_SHADER_MAX_STAGES] = {};
        for (u32 i = 0; i < vulkanShader->config.stageCount; i++)
        {
            stageCreateInfos[i] = vulkanShader->stages[i].shaderStageCreateInfo;
        }

        // Create one pipeline per topology class if dynamic topology is supported (either natively or by extension)
        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE) ||
            m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            // Total of 3 topology classes
            vulkanShader->pipelines.Resize(3);

            // Point class
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST)
            {
                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_POINT] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST);
            }

            // Line class
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST ||
                vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP)
            {
                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_LINE] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST | PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP);
            }

            // Triangle class
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST ||
                vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP ||
                vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN)
            {
                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_TRIANGLE] = Memory.New<VulkanPipeline>(
                    MemoryType::Vulkan,
                    PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST | PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP | PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN);
            }
        }
        else
        {
            // We have no support for dynamic topology so we need to create a pipeline per topology type (6 in total)
            // We also need to create seperate pipelines for clockwise and counter-clockwise since this is also not supported without
            // extended dynamic state
            vulkanShader->pipelines.Resize(6);
            vulkanShader->clockwisePipelines.Resize(6);

            // Point list
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST)
            {
                // Counter-clockwise
                vulkanShader->pipelines[0] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST);
                // Clockwise
                vulkanShader->clockwisePipelines[0] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST, RendererWinding::Clockwise);
            }

            // Line list
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST)
            {
                // Counter-clockwise
                vulkanShader->pipelines[1] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST);
                // Clockwise
                vulkanShader->clockwisePipelines[1] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST, RendererWinding::Clockwise);
            }
            // Line strip
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP)
            {
                // Counter-clockwise
                vulkanShader->pipelines[2] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP);
                // Clockwise
                vulkanShader->clockwisePipelines[2] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP, RendererWinding::Clockwise);
            }

            // Triangle list
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST)
            {
                // Counter-clockwise
                vulkanShader->pipelines[3] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST);
                // Clockwise
                vulkanShader->clockwisePipelines[3] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST, RendererWinding::Clockwise);
            }
            // Triangle strip
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP)
            {
                // Counter-clockwise
                vulkanShader->pipelines[4] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP);
                // Clockwise
                vulkanShader->clockwisePipelines[4] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP, RendererWinding::Clockwise);
            }
            // Triangle fan
            if (vulkanShader->config.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN)
            {
                // Counter-clockwise
                vulkanShader->pipelines[5] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN);
                // Clockwise
                vulkanShader->clockwisePipelines[5] =
                    Memory.New<VulkanPipeline>(MemoryType::Vulkan, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN, RendererWinding::Clockwise);
            }
        }

        for (u32 i = 0; i < VULKAN_TOPOLOGY_CLASS_MAX; i++)
        {
            if (!vulkanShader->pipelines[i]) continue;

            VulkanPipelineConfig config = {};
            config.attributes.Copy(vulkanShader->config.attributes, shader.attributes.Size());
            config.pushConstantRanges.Copy(shader.pushConstantRanges, shader.pushConstantRangeCount);
            config.descriptorSetLayouts.Copy(vulkanShader->descriptorSetLayouts, vulkanShader->config.descriptorSetCount);
            config.stages.Copy(stageCreateInfos, vulkanShader->config.stageCount);
            config.renderPass  = vulkanShader->renderPass;
            config.stride      = shader.attributeStride;
            config.viewport    = viewport;
            config.scissor     = scissor;
            config.cullMode    = vulkanShader->config.cullMode;
            config.shaderFlags = shader.flags;
            config.shaderName  = shader.name;

            if (vulkanShader->boundPipeline == INVALID_ID_U8)
            {
                // Set the bound pipeline to the first valid pipeline
                vulkanShader->boundPipeline = i;
            }

            if (!vulkanShader->pipelines[i]->Create(&m_context, config))
            {
                ERROR_LOG("Failed to create pipeline for topology type: '{}'.", i);
                return false;
            }
        }

        if (vulkanShader->boundPipeline == INVALID_ID_U8)
        {
            ERROR_LOG("No valid bound pipeline for shader.");
            return false;
        }

        // Grab the UBO alignment requirement from our device
        shader.requiredUboAlignment = m_context.device.GetMinUboAlignment();

        // Make sure the UBO is aligned according to device requirements
        shader.globalUboStride = GetAligned(shader.globalUboSize, shader.requiredUboAlignment);
        shader.uboStride       = GetAligned(shader.uboSize, shader.requiredUboAlignment);

        // Uniform buffer
        // TODO: max count should be configurable, or perhaps long term support of buffer resizing
        const u64 totalBufferSize = shader.globalUboStride + shader.uboStride * VULKAN_MAX_MATERIAL_COUNT;
        if (!vulkanShader->uniformBuffer.Create(RenderBufferType::Uniform, totalBufferSize, RenderBufferTrackType::FreeList))
        {
            ERROR_LOG("Failed to create VulkanBuffer.");
            return false;
        }
        vulkanShader->uniformBuffer.Bind(0);

        // Allocate space for the global UBO, which should occupy the stride space and not the actual size need
        if (!vulkanShader->uniformBuffer.Allocate(shader.globalUboStride, shader.globalUboOffset))
        {
            ERROR_LOG("Failed to allocate space for the uniform buffer.");
            return false;
        }

        // Map the entire buffer's memory
        vulkanShader->mappedUniformBufferBlock = vulkanShader->uniformBuffer.MapMemory(0, VK_WHOLE_SIZE);

        const VkDescriptorSetLayout globalLayouts[3] = {
            vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
            vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
            vulkanShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
        };

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool              = vulkanShader->descriptorPool;
        allocInfo.descriptorSetCount          = 3;
        allocInfo.pSetLayouts                 = globalLayouts;
        VK_CHECK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, vulkanShader->globalDescriptorSets));

        return true;
    }

    bool VulkanRendererPlugin::UseShader(const Shader& shader)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);
        vulkanShader->pipelines[vulkanShader->boundPipeline]->Bind(&m_context.graphicsCommandBuffers[m_context.imageIndex],
                                                                   VK_PIPELINE_BIND_POINT_GRAPHICS);

        m_context.boundShader = &shader;
        return true;
    }

    bool VulkanRendererPlugin::BindShaderGlobals(Shader& shader)
    {
        // Global UBO is always at the beginning, but let's use this anyways for completeness
        shader.boundUboOffset = static_cast<u32>(shader.globalUboOffset);
        return true;
    }

    bool VulkanRendererPlugin::BindShaderInstance(Shader& shader, const u32 instanceId)
    {
        const auto internal = static_cast<VulkanShader*>(shader.apiSpecificData);

        const VulkanShaderInstanceState* instanceState = &internal->instanceStates[instanceId];

        shader.boundUboOffset = static_cast<u32>(instanceState->offset);
        return true;
    }

    bool VulkanRendererPlugin::ShaderApplyGlobals(const Shader& shader, bool needsUpdate)
    {
        const u32 imageIndex = m_context.imageIndex;
        const auto internal  = static_cast<VulkanShader*>(shader.apiSpecificData);

        VkCommandBuffer commandBuffer    = m_context.graphicsCommandBuffers[imageIndex].handle;
        VkDescriptorSet globalDescriptor = internal->globalDescriptorSets[imageIndex];

        if (needsUpdate)
        {
            // Apply UBO first
            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = internal->uniformBuffer.handle;
            bufferInfo.offset = shader.globalUboOffset;
            bufferInfo.range  = shader.globalUboStride;

            // Update descriptor sets
            VkWriteDescriptorSet uboWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            uboWrite.dstSet               = internal->globalDescriptorSets[imageIndex];
            uboWrite.dstBinding           = 0;
            uboWrite.dstArrayElement      = 0;
            uboWrite.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount      = 1;
            uboWrite.pBufferInfo          = &bufferInfo;

            VkWriteDescriptorSet descriptorWrites[2];
            descriptorWrites[0] = uboWrite;

            u32 globalSetBindingCount = internal->config.descriptorSets[DESC_SET_INDEX_GLOBAL].bindingCount;
            if (globalSetBindingCount > 1)
            {
                // TODO: There are samplers to be written.
                globalSetBindingCount = 1;
                ERROR_LOG("Global image samplers are not yet supported.");
            }

            vkUpdateDescriptorSets(m_context.device.GetLogical(), globalSetBindingCount, descriptorWrites, 0, nullptr);
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal->pipelines[internal->boundPipeline]->layout, 0, 1,
                                &globalDescriptor, 0, nullptr);
        return true;
    }

    bool VulkanRendererPlugin::ShaderApplyInstance(const Shader& shader, const bool needsUpdate)
    {
        const auto internal = static_cast<VulkanShader*>(shader.apiSpecificData);
        if (internal->instanceUniformCount == 0 && internal->instanceUniformSamplerCount == 0)
        {
            ERROR_LOG("This shader does not use instances.");
            return false;
        }

        const u32 imageIndex          = m_context.imageIndex;
        VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[imageIndex].handle;

        // Obtain instance data
        VulkanShaderInstanceState* objectState = &internal->instanceStates[shader.boundInstanceId];
        VkDescriptorSet objectDescriptorSet    = objectState->descriptorSetState.descriptorSets[imageIndex];

        // We only update if it is needed
        if (needsUpdate)
        {
            VkWriteDescriptorSet descriptorWrites[2] = {};  // Always a max of 2 descriptor sets
            u32 descriptorCount                      = 0;
            u32 descriptorIndex                      = 0;

            VkDescriptorBufferInfo bufferInfo;

            // Descriptor 0 - Uniform buffer
            if (internal->instanceUniformCount > 0)
            {
                // Only do this if the descriptor has not yet been updated
                u8& instanceUboGeneration = objectState->descriptorSetState.descriptorStates[descriptorIndex].generations[imageIndex];
                if (instanceUboGeneration == INVALID_ID_U8)
                {
                    bufferInfo.buffer = internal->uniformBuffer.handle;
                    bufferInfo.offset = objectState->offset;
                    bufferInfo.range  = shader.uboStride;

                    VkWriteDescriptorSet uboDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    uboDescriptor.dstSet               = objectDescriptorSet;
                    uboDescriptor.dstBinding           = descriptorIndex;
                    uboDescriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    uboDescriptor.descriptorCount      = 1;
                    uboDescriptor.pBufferInfo          = &bufferInfo;

                    descriptorWrites[descriptorCount] = uboDescriptor;
                    descriptorCount++;

                    // TODO: some generation from... somewhere
                    instanceUboGeneration = 1;
                }
                descriptorIndex++;
            }

            // Iterate samplers.
            if (internal->instanceUniformSamplerCount > 0)
            {
                const auto samplerBindingIndex = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].samplerBindingIndex;
                const u32 totalSamplerCount =
                    internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindings[samplerBindingIndex].descriptorCount;
                u32 updateSamplerCount = 0;
                VkDescriptorImageInfo imageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
                for (u32 i = 0; i < totalSamplerCount; i++)
                {
                    // TODO: only update in the list if actually needing an update
                    TextureMap* map = internal->instanceStates[shader.boundInstanceId].instanceTextureMaps[i];
                    if (map->internalId == INVALID_ID)
                    {
                        // No valid sampler available so we skip this texture map.
                        continue;
                    }

                    const Texture* t = map->texture;
                    // Ensure the texture is valid.
                    if (!t || t->generation == INVALID_ID)
                    {
                        // If we are using the default texture, invalidate the map's generation so it's updated next run.
                        t               = Textures.GetDefault();
                        map->generation = INVALID_ID;
                    }
                    else
                    {
                        // If the texture is valid, we ensure that the texture map's generation matches the texture.
                        // If not, the texture map resources should be regenerated
                        if (t->generation != map->generation)
                        {
                            bool refreshRequired = t->mipLevels != map->mipLevels;
                            if (refreshRequired && !RefreshTextureMapResources(*map))
                            {
                                WARN_LOG("Failed to refresh texture map resources. This means the sampler settings could be out of date!");
                            }
                            else
                            {
                                map->generation = t->generation;
                            }
                        }
                    }

                    const auto internalData   = static_cast<VulkanTextureData*>(t->internalData);
                    imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[i].imageView   = internalData->image.view;
                    imageInfos[i].sampler     = m_context.samplers[map->internalId];

                    // TODO: change up descriptor state to handle this properly.
                    // Sync frame generation if not using a default texture.
                    updateSamplerCount++;
                }

                if (updateSamplerCount > 0)
                {
                    VkWriteDescriptorSet samplerDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    samplerDescriptor.dstSet               = objectDescriptorSet;
                    samplerDescriptor.dstBinding           = descriptorIndex;
                    samplerDescriptor.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    samplerDescriptor.descriptorCount      = updateSamplerCount;
                    samplerDescriptor.pImageInfo           = imageInfos;

                    descriptorWrites[descriptorCount] = samplerDescriptor;
                    descriptorCount++;
                }
            }

            if (descriptorCount > 0)
            {
                vkUpdateDescriptorSets(m_context.device.GetLogical(), descriptorCount, descriptorWrites, 0, nullptr);
            }
        }

        // We always bind for every instance however
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, internal->pipelines[internal->boundPipeline]->layout, 1, 1,
                                &objectDescriptorSet, 0, nullptr);
        return true;
    }

    VkSamplerAddressMode VulkanRendererPlugin::ConvertRepeatType(const char* axis, const TextureRepeat repeat) const
    {
        switch (repeat)
        {
            case TextureRepeat::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureRepeat::MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case TextureRepeat::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureRepeat::ClampToBorder:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            default:
                WARN_LOG("Axis = '{}', TextureRepeat = '{}' is not supported. Defaulting to repeat.", axis, ToUnderlying(repeat));
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    VkFilter VulkanRendererPlugin::ConvertFilterType(const char* op, const TextureFilter filter) const
    {
        switch (filter)
        {
            case TextureFilter::ModeNearest:
                return VK_FILTER_NEAREST;
            case TextureFilter::ModeLinear:
                return VK_FILTER_LINEAR;
            default:
                WARN_LOG("Op = '{}', Filter = '{}' is not supported. Defaulting to linear.", op, ToUnderlying(filter));
                return VK_FILTER_LINEAR;
        }
    }

    bool VulkanRendererPlugin::AcquireShaderInstanceResources(const Shader& shader, u32 textureMapCount, const TextureMap** maps,
                                                              u32* outInstanceId)
    {
        const auto internal = static_cast<VulkanShader*>(shader.apiSpecificData);
        // TODO: dynamic
        *outInstanceId = INVALID_ID;
        for (u32 i = 0; i < VULKAN_MAX_MATERIAL_COUNT; i++)
        {
            if (internal->instanceStates[i].id == INVALID_ID)
            {
                internal->instanceStates[i].id = i;
                *outInstanceId                 = i;
                break;
            }
        }

        if (*outInstanceId == INVALID_ID)
        {
            ERROR_LOG("Failed to acquire new id.");
            return false;
        }

        VulkanShaderInstanceState* instanceState = &internal->instanceStates[*outInstanceId];
        // Only setup if the shader actually requires it
        if (shader.instanceTextureCount > 0)
        {
            // Wipe out the memory for the entire array, even if it isn't all used.
            instanceState->instanceTextureMaps = Memory.Allocate<TextureMap*>(MemoryType::Array, shader.instanceTextureCount);
            Texture* defaultTexture            = Textures.GetDefault();
            std::memcpy(instanceState->instanceTextureMaps, maps, sizeof(TextureMap*) * textureMapCount);
            // Set unassigned texture pointers to default until assigned
            for (u32 i = 0; i < textureMapCount; i++)
            {
                if (maps[i] && !maps[i]->texture)
                {
                    instanceState->instanceTextureMaps[i]->texture = defaultTexture;
                }
            }
        }

        // Allocate some space in the UBO - by the stride, not the size
        const u64 size = shader.uboStride;
        if (size > 0)
        {
            if (!internal->uniformBuffer.Allocate(size, instanceState->offset))
            {
                ERROR_LOG("Failed to acquire UBO space.");
                return false;
            }
        }

        VulkanShaderDescriptorSetState* setState = &instanceState->descriptorSetState;

        // Each descriptor binding in the set
        const u32 bindingCount = internal->config.descriptorSets[DESC_SET_INDEX_INSTANCE].bindingCount;
        std::memset(setState->descriptorStates, 0, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);
        for (u32 i = 0; i < bindingCount; i++)
        {
            for (u32 j = 0; j < 3; j++)
            {
                setState->descriptorStates[i].generations[j] = INVALID_ID_U8;
                setState->descriptorStates[i].ids[j]         = INVALID_ID;
            }
        }

        // Allocate 3 descriptor sets (one per frame)
        const VkDescriptorSetLayout layouts[3] = {
            internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
            internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
            internal->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
        };

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool              = internal->descriptorPool;
        allocInfo.descriptorSetCount          = 3;
        allocInfo.pSetLayouts                 = layouts;
        const VkResult result =
            vkAllocateDescriptorSets(m_context.device.GetLogical(), &allocInfo, instanceState->descriptorSetState.descriptorSets);
        if (result != VK_SUCCESS)
        {
            ERROR_LOG("Error allocating descriptor sets in Shader: '{}'.", VulkanUtils::ResultString(result));
            return false;
        }
        return true;
    }

    bool VulkanRendererPlugin::ReleaseShaderInstanceResources(const Shader& shader, const u32 instanceId)
    {
        const auto internal                      = static_cast<VulkanShader*>(shader.apiSpecificData);
        VulkanShaderInstanceState* instanceState = &internal->instanceStates[instanceId];

        // Wait for any pending operations using the descriptor set to finish
        m_context.device.WaitIdle();

        // Free 3 descriptor sets (one per frame)
        const VkResult result = vkFreeDescriptorSets(m_context.device.GetLogical(), internal->descriptorPool, 3,
                                                     instanceState->descriptorSetState.descriptorSets);

        if (result != VK_SUCCESS)
        {
            ERROR_LOG("Error while freeing shader descriptor sets.");
        }

        // Destroy descriptor states
        std::memset(instanceState->descriptorSetState.descriptorStates, 0, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);

        // Free the memory for the instance texture pointer array
        if (instanceState->instanceTextureMaps)
        {
            Memory.Free(instanceState->instanceTextureMaps);
            instanceState->instanceTextureMaps = nullptr;
        }

        if (shader.uboStride != 0)
        {
            internal->uniformBuffer.Free(shader.uboStride, instanceState->offset);
        }
        instanceState->offset = INVALID_ID;
        instanceState->id     = INVALID_ID;

        return true;
    }

    bool VulkanRendererPlugin::AcquireTextureMapResources(TextureMap& map)
    {
        u32 selectedSamplerIndex = INVALID_ID;
        // Find a free sampler slot
        for (u32 i = 0; i < m_context.samplers.Size(); i++)
        {
            if (!m_context.samplers[i])
            {
                // We have found an empty slot
                selectedSamplerIndex = i;
            }
        }

        if (selectedSamplerIndex == INVALID_ID)
        {
            // We could not find an empty sampler slot so we add a new one
            selectedSamplerIndex = m_context.samplers.Size();
            m_context.samplers.PushBack(nullptr);
        }

        // Create our sampler at the selected index
        m_context.samplers[selectedSamplerIndex] = CreateSampler(map);
        if (!m_context.samplers[selectedSamplerIndex])
        {
            ERROR_LOG("Failed to create Sampler.");
            return false;
        }

        const auto samplerName = map.texture->name + "_texture_map_sampler";
        VK_SET_DEBUG_OBJECT_NAME(&m_context, VK_OBJECT_TYPE_SAMPLER, m_context.samplers[selectedSamplerIndex], samplerName);

        // Assign our sampler index to the internal id of our texture map so we can find the sampler later for use
        map.internalId = selectedSamplerIndex;
        return true;
    }

    void VulkanRendererPlugin::ReleaseTextureMapResources(TextureMap& map)
    {
        if (map.internalId != INVALID_ID)
        {  // Ensure there's the texture map resources (sampler) is in use
            m_context.device.WaitIdle();
            // Destroy our sampler
            vkDestroySampler(m_context.device.GetLogical(), m_context.samplers[map.internalId], m_context.allocator);
            // Free up the sampler slot in our array
            m_context.samplers[map.internalId] = nullptr;
            // Ensure that the texture map no longer links to the sampler that we just destroyed
            map.internalId = INVALID_ID;
        }
    }

    bool VulkanRendererPlugin::RefreshTextureMapResources(TextureMap& map)
    {
        if (map.internalId != INVALID_ID)
        {
            // Create a new sampler first
            VkSampler newSampler = CreateSampler(map);
            if (!newSampler)
            {
                ERROR_LOG("Failed to create new Sampler.");
                return false;
            }

            // Take a copy of the old sampler
            auto oldSampler = m_context.samplers[map.internalId];
            // Ensure we are not using the current sampler first
            m_context.device.WaitIdle();
            // Assign our new sampler
            m_context.samplers[map.internalId] = newSampler;
            // Destroy the old sampler
            vkDestroySampler(m_context.device.GetLogical(), oldSampler, m_context.allocator);
        }

        return true;
    }

    bool VulkanRendererPlugin::SetUniform(Shader& shader, const ShaderUniform& uniform, const void* value)
    {
        const auto internal = static_cast<VulkanShader*>(shader.apiSpecificData);
        if (uniform.type == Uniform_Sampler)
        {
            if (uniform.scope == ShaderScope::Global)
            {
                shader.globalTextureMaps[uniform.location] = (TextureMap*)value;
            }
            else
            {
                internal->instanceStates[shader.boundInstanceId].instanceTextureMaps[uniform.location] = (TextureMap*)value;
            }
        }
        else
        {
            if (uniform.scope == ShaderScope::Local)
            {
                // Is local, using push constants. Do this immediately.
                VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex].handle;
                vkCmdPushConstants(commandBuffer, internal->pipelines[internal->boundPipeline]->layout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<u32>(uniform.offset),
                                   uniform.size, value);
            }
            else
            {
                // Map the appropriate memory location and copy the data over.
                auto address = static_cast<u8*>(internal->mappedUniformBufferBlock);
                address += shader.boundUboOffset + uniform.offset;
                std::memcpy(address, value, uniform.size);
            }
        }

        return true;
    }

    void VulkanRendererPlugin::CreateCommandBuffers()
    {
        if (m_context.graphicsCommandBuffers.Empty())
        {
            m_context.graphicsCommandBuffers.Resize(m_context.swapChain.imageCount);
        }

        auto graphicsCommandPool = m_context.device.GetGraphicsCommandPool();
        for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
        {
            if (m_context.graphicsCommandBuffers[i].handle)
            {
                m_context.graphicsCommandBuffers[i].Free(&m_context, graphicsCommandPool);
            }

            m_context.graphicsCommandBuffers[i].Allocate(&m_context, graphicsCommandPool, true);
        }
    }

    bool VulkanRendererPlugin::RecreateSwapChain()
    {
        if (m_context.recreatingSwapChain)
        {
            DEBUG_LOG("Called when already recreating.");
            return false;
        }

        if (m_context.frameBufferWidth == 0 || m_context.frameBufferHeight == 0)
        {
            DEBUG_LOG("Called when at least one of the window dimensions is < 1.");
            return false;
        }

        m_context.recreatingSwapChain = true;

        // Ensure that our device is not busy
        m_context.device.WaitIdle();

        // Clear out all the in-flight images since the size of the FrameBuffer will change
        for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
        {
            m_context.imagesInFlight[i] = nullptr;
        }

        // Re-query the SwapChain support and depth format since it might have changed
        m_context.device.QuerySwapChainSupport();
        m_context.device.DetectDepthFormat();

        m_context.swapChain.Recreate(m_context.frameBufferWidth, m_context.frameBufferHeight, m_config.flags);

        // Update the size generation so that they are in sync again
        m_context.frameBufferSizeLastGeneration = m_context.frameBufferSizeGeneration;

        // Cleanup SwapChain
        auto graphicsCommandPool = m_context.device.GetGraphicsCommandPool();
        for (u32 i = 0; i < m_context.swapChain.imageCount; i++)
        {
            m_context.graphicsCommandBuffers[i].Free(&m_context, graphicsCommandPool);
        }

        // Tell the renderer that a refresh is required
        Event.Fire(EventCodeDefaultRenderTargetRefreshRequired, nullptr, {});

        CreateCommandBuffers();

        m_context.recreatingSwapChain = false;
        return true;
    }

    bool VulkanRendererPlugin::CreateShaderModule(const VulkanShaderStageConfig& config, VulkanShaderStage* shaderStage) const
    {
        // Read the resource
        TextResource res{};
        if (!Resources.Load(config.fileName, res))
        {
            ERROR_LOG("Unable to read Shader Module: '{}'.", config.fileName);
            return false;
        }

        shaderc_shader_kind shaderKind;
        switch (config.stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                shaderKind = shaderc_glsl_default_vertex_shader;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                shaderKind = shaderc_glsl_default_fragment_shader;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                shaderKind = shaderc_glsl_default_compute_shader;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                shaderKind = shaderc_glsl_default_geometry_shader;
                break;
            default:
                ERROR_LOG("Unsupported shader kind. Unable to create ShaderModule.");
                return false;
        }

        INFO_LOG("Compiling: '{}' Stage for ShaderModule: '{}'.", ToString(config.stage), config.fileName);

        // Attempt to compile the shader
        shaderc_compile_options_t compileOptions = shaderc_compile_options_initialize();
        if (!compileOptions)
        {
            ERROR_LOG("Failed to initialize compile options for ShaderModuel: '{}'.", config.fileName);
            return false;
        }

        shaderc_compile_options_set_optimization_level(compileOptions, shaderc_optimization_level_performance);

        shaderc_compilation_result_t compilationResult = shaderc_compile_into_spv(
            m_context.shaderCompiler, res.text.Data(), res.text.Size(), shaderKind, config.fileName.Data(), "main", compileOptions);

        // Release our resource
        Resources.Unload(res);

        if (!compilationResult)
        {
            ERROR_LOG("Unknown error while trying to compile stage for ShaderModule: '{}'.", config.fileName);
            return false;
        }

        shaderc_compilation_status status = shaderc_result_get_compilation_status(compilationResult);

        // Handle errors if there are any
        if (status != shaderc_compilation_status_success)
        {
            const char* errorMessage = shaderc_result_get_error_message(compilationResult);
            u64 errorCount           = shaderc_result_get_num_errors(compilationResult);

            ERROR_LOG("Compiling ShaderModule: '{}' failed with {} error(s).", config.fileName, errorCount);
            ERROR_LOG("Errors:\n{}", errorMessage);

            shaderc_result_release(compilationResult);
            return false;
        }

        // Output warnings if there are any.
        u64 warningCount = shaderc_result_get_num_warnings(compilationResult);
        if (warningCount > 0)
        {
            // NOTE: Not sure this it the correct way to obtain warnings.
            const char* warnings = shaderc_result_get_error_message(compilationResult);
            WARN_LOG("Found: {} warnings while compiling ShaderModule: '{}':\n{}", warningCount, config.fileName, warnings);
        }

        // Extract the data from the result.
        const char* bytes = shaderc_result_get_bytes(compilationResult);
        u64 byteCount     = shaderc_result_get_length(compilationResult);

        // Take a copy of the result data and cast it to a u32* as is required by Vulkan.
        u32* code = Memory.Allocate<u32>(C3D::MemoryType::RenderSystem, byteCount);
        std::memcpy(code, bytes, byteCount);

        // Release the compilation result.
        shaderc_result_release(compilationResult);

        INFO_LOG("Successfully compiled: '{}' Stage consisting of {} bytes for ShaderModule: '{}'.", ToString(config.stage), byteCount,
                 config.fileName);

        shaderStage->createInfo          = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shaderStage->createInfo.codeSize = byteCount;
        shaderStage->createInfo.pCode    = code;

        VK_CHECK(vkCreateShaderModule(m_context.device.GetLogical(), &shaderStage->createInfo, m_context.allocator, &shaderStage->handle));

        // Release our allocated memory again since the ShaderModule has been created
        Memory.Free(code);

        VK_SET_DEBUG_OBJECT_NAME(&m_context, VK_OBJECT_TYPE_SHADER_MODULE, shaderStage->handle, res.name);

        // Shader stage info
        shaderStage->shaderStageCreateInfo        = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shaderStage->shaderStageCreateInfo.stage  = config.stage;
        shaderStage->shaderStageCreateInfo.module = shaderStage->handle;
        // TODO: make this configurable?
        shaderStage->shaderStageCreateInfo.pName = "main";

        return true;
    }

    VkSampler VulkanRendererPlugin::CreateSampler(TextureMap& map)
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

        // Sync mip levels between texture and texturemap
        map.mipLevels = map.texture->mipLevels;

        samplerInfo.minFilter = ConvertFilterType("min", map.minifyFilter);
        samplerInfo.magFilter = ConvertFilterType("mag", map.magnifyFilter);

        samplerInfo.addressModeU = ConvertRepeatType("U", map.repeatU);
        samplerInfo.addressModeV = ConvertRepeatType("V", map.repeatV);
        samplerInfo.addressModeW = ConvertRepeatType("W", map.repeatW);

        // TODO: Configurable
        samplerInfo.anisotropyEnable        = VK_TRUE;
        samplerInfo.maxAnisotropy           = 16;
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias              = 0.0f;
        // Use the full range of mips available
        samplerInfo.minLod = 0.0f;
        // samplerInfo.minLod = map.texture->mipLevels > 1 ? map.texture->mipLevels : 0.0f;
        samplerInfo.maxLod = map.texture->mipLevels;

        VkSampler sampler     = nullptr;
        const VkResult result = vkCreateSampler(m_context.device.GetLogical(), &samplerInfo, m_context.allocator, &sampler);
        if (!VulkanUtils::IsSuccess(result))
        {
            ERROR_LOG("Error creating texture sampler: '{}'.", VulkanUtils::ResultString(result));
            return nullptr;
        }

        return sampler;
    }

    RendererPlugin* CreatePlugin() { return Memory.New<VulkanRendererPlugin>(MemoryType::RenderSystem); }

    void DeletePlugin(RendererPlugin* plugin) { Memory.Delete(plugin); }

}  // namespace C3D
