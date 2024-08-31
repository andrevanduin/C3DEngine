
#include "vulkan_renderer_plugin.h"

#include <engine.h>
#include <metrics/metrics.h>
#include <logger/logger.h>
#include <platform/platform.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/renderer_utils.h>
#include <renderer/vertex.h>
#include <resources/managers/text_manager.h>
#include <resources/shaders/shader.h>
#include <resources/textures/texture.h>
#include <shaderc/shaderc.h>
#include <shaderc/status.h>
#include <systems/events/event_context.h>
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

        if (config.flags & RendererConfigFlagBits::FlagUseValidationLayers)
        {
            m_context.useValidationLayers = true;
            INFO_LOG("Validation layers are requested.");
        }

        // Just set some basic default values. They will be overridden anyway.
        m_context.frameBufferWidth  = 1280;
        m_context.frameBufferHeight = 720;
        m_config                    = config;

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

        if (!VulkanPlatform::CreateSurface(m_context))
        {
            ERROR_LOG("Failed to create Vulkan Surface.");
            return false;
        }

        if (!m_context.device.Create(&m_context))
        {
            ERROR_LOG("Failed to create Vulkan Device.");
            return false;
        }

        m_context.swapchain.Create(&m_context, m_context.frameBufferWidth, m_context.frameBufferHeight, config.flags);

        // Save the number of images we have as a the number of render targets required
        *outWindowRenderTargetCount = static_cast<u8>(m_context.swapchain.imageCount);

        CreateCommandBuffers();
        INFO_LOG("Command Buffers Initialized.");

        m_context.imageAvailableSemaphores.Resize(m_context.swapchain.maxFramesInFlight);
        m_context.queueCompleteSemaphores.Resize(m_context.swapchain.maxFramesInFlight);

        INFO_LOG("Creating Semaphores and Fences.");
        auto logicalDevice = m_context.device.GetLogical();
        for (u8 i = 0; i < m_context.swapchain.maxFramesInFlight; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.imageAvailableSemaphores[i]);
            vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, m_context.allocator, &m_context.queueCompleteSemaphores[i]);

            VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceCreateInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(logicalDevice, &fenceCreateInfo, m_context.allocator, &m_context.inFlightFences[i]));
        }

        for (u32 i = 0; i < m_context.swapchain.imageCount; i++)
        {
            m_context.imagesInFlight[i] = nullptr;
        }

        constexpr u64 stagingBufferSize = MebiBytes(512);
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
        for (u8 i = 0; i < m_context.swapchain.maxFramesInFlight; i++)
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
        m_context.swapchain.Destroy();

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
        if (!m_context.swapchain.AcquireNextImageIndex(UINT64_MAX, m_context.imageAvailableSemaphores[m_context.currentFrame], nullptr,
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

        // TODO: Might want a semaphore here to monitor timing
        // For timing purposes, wait for the queue to complete.
        // This gives an accurate picture of how long the render takes, including the work submitted to the actual queue.
        // vkWaitForFences(m_context.device.GetLogical(), 1, &m_context.inFlightFences[m_context.currentFrame], true, UINT64_MAX);

        return true;
    }

    bool VulkanRendererPlugin::Present(const FrameData& frameData)
    {
        // Present the image (and give it back to the SwapChain)
        auto presentQueue = m_context.device.GetPresentQueue();
        m_context.swapchain.Present(presentQueue, m_context.queueCompleteSemaphores[m_context.currentFrame], m_context.imageIndex);

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
        else
        {
            // Support by means of extension
            m_context.pfnCmdSetFrontFaceEXT(commandBuffer.handle, frontFace);
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

    void VulkanRendererPlugin::CreateRenderTarget(void* pass, RenderTarget& target, u16 layerIndex, u32 width, u32 height)
    {
        VkImageView attachmentViews[32] = { 0 };
        for (u32 i = 0; i < target.attachments.Size(); i++)
        {
            auto internal = Textures.GetInternals<VulkanImage>(target.attachments[i].texture);
            if (!internal->layerViews.Empty())
            {
                attachmentViews[i] = internal->layerViews[layerIndex];
            }
            else
            {
                attachmentViews[i] = internal->view;
            }
        }

        // Setup our frameBuffer creation
        VkFramebufferCreateInfo frameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        frameBufferCreateInfo.renderPass              = static_cast<VulkanRenderpass*>(pass)->handle;
        frameBufferCreateInfo.attachmentCount         = target.attachments.Size();
        frameBufferCreateInfo.pAttachments            = attachmentViews;
        frameBufferCreateInfo.width                   = width;
        frameBufferCreateInfo.height                  = height;
        frameBufferCreateInfo.layers                  = 1;

        VK_CHECK(vkCreateFramebuffer(m_context.device.GetLogical(), &frameBufferCreateInfo, m_context.allocator,
                                     reinterpret_cast<VkFramebuffer*>(&target.internalFrameBuffer)));

        auto vulkanPass = static_cast<VulkanRenderpass*>(pass);
        const auto name = String::FromFormat("PASS_{}_FRAMEBUFFER_{}x{}", vulkanPass->GetName(), width, height);
        VK_SET_DEBUG_OBJECT_NAME(&m_context, VK_OBJECT_TYPE_FRAMEBUFFER, target.internalFrameBuffer, name);
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

    bool VulkanRendererPlugin::CreateRenderpassInternals(const RenderpassConfig& config, void** internalData)
    {
        auto pass = Memory.New<VulkanRenderpass>(MemoryType::RenderSystem);
        if (!pass->Create(config, &m_context))
        {
            Memory.Delete(pass);
            ERROR_LOG("Failed to create Renderpass internal data for: '{}'.", config.name);
            return false;
        }

        *internalData = pass;
        return true;
    }

    void VulkanRendererPlugin::DestroyRenderpassInternals(void* internalData)
    {
        auto pass = static_cast<VulkanRenderpass*>(internalData);
        pass->Destroy();
        Memory.Delete(pass);
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

    void VulkanRendererPlugin::WaitForIdle()
    {
        auto result = m_context.device.WaitIdle();
        if (!VulkanUtils::IsSuccess(result))
        {
            ERROR_LOG("Failed to wait for device idle: '{}'.", VulkanUtils::ResultString(result));
        }
    }

    void VulkanRendererPlugin::BeginDebugLabel(const String& text, const vec3& color)
    {
#ifdef _DEBUG
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
        vec4 rgba           = vec4(color, 1.0f);
#endif
        VK_BEGIN_CMD_DEBUG_LABEL(&m_context, commandBuffer.handle, text, rgba);
    }

    void VulkanRendererPlugin::EndDebugLabel()
    {
#ifdef _DEBUG
        auto& commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex];
#endif
        VK_END_CMD_DEBUG_LABEL(&m_context, commandBuffer.handle);
    }

    TextureHandle VulkanRendererPlugin::GetWindowAttachment(const u8 index)
    {
        if (index >= m_context.swapchain.imageCount)
        {
            FATAL_LOG("Attempting to get attachment index that is out of range: '{}'. Attachment count is: '{}'.", index,
                      m_context.swapchain.imageCount);
        }
        return m_context.swapchain.renderTextures[index].handle;
    }

    TextureHandle VulkanRendererPlugin::GetDepthAttachment(u8 index)
    {
        if (index >= m_context.swapchain.imageCount)
        {
            FATAL_LOG("Attempting to get attachment index that is out of range: '{}'. Attachment count is: '{}'.", index,
                      m_context.swapchain.imageCount);
        }
        return m_context.swapchain.depthTextures[index].handle;
    }

    u8 VulkanRendererPlugin::GetWindowAttachmentIndex() { return static_cast<u8>(m_context.imageIndex); }

    u8 VulkanRendererPlugin::GetWindowAttachmentCount() { return static_cast<u8>(m_context.swapchain.imageCount); }

    bool VulkanRendererPlugin::IsMultiThreaded() const { return m_context.multiThreadingEnabled; }

    void VulkanRendererPlugin::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled)
    {
        m_config.flags              = enabled ? (m_config.flags | flag) : (m_config.flags & ~flag);
        m_context.renderFlagChanged = true;
    }

    bool VulkanRendererPlugin::IsFlagEnabled(const RendererConfigFlagBits flag) const { return m_config.flags & flag; }

    void VulkanRendererPlugin::BeginRenderpass(void* pass, const Viewport* viewport, const RenderTarget& target)
    {
        VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
        const auto vulkanPass              = static_cast<VulkanRenderpass*>(pass);
        vulkanPass->Begin(commandBuffer, viewport, target);
    }

    void VulkanRendererPlugin::EndRenderpass(void* pass)
    {
        VulkanCommandBuffer* commandBuffer = &m_context.graphicsCommandBuffers[m_context.imageIndex];
        const auto vulkanPass              = static_cast<VulkanRenderpass*>(pass);
        vulkanPass->End(commandBuffer);
    }

    void VulkanRendererPlugin::CreateTexture(Texture& texture, const u8* pixels)
    {
        // Internal data creation
        texture.internalData = Memory.New<VulkanImage>(MemoryType::Texture);

        const auto image             = static_cast<VulkanImage*>(texture.internalData);
        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture.width * texture.height * texture.channelCount * texture.arraySize);

        // NOTE: Assumes 8 bits per channel
        constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        image->Create(&m_context, texture.name, texture.type, texture.width, texture.height, texture.arraySize, imageFormat,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture.mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);

        // Load the data
        WriteDataToTexture(texture, 0, static_cast<u32>(imageSize), pixels, false);
        // Increment the generation since we made changes
        texture.generation++;
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

    void VulkanRendererPlugin::CreateWritableTexture(Texture& texture)
    {
        texture.internalData = Memory.New<VulkanImage>(MemoryType::Texture);
        const auto image     = static_cast<VulkanImage*>(texture.internalData);

        VkFormat imageFormat;
        VkImageUsageFlagBits usage;
        VkImageAspectFlagBits aspect;

        if (texture.flags & TextureFlag::IsDepth)
        {
            usage       = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            aspect      = VK_IMAGE_ASPECT_DEPTH_BIT;
            imageFormat = m_context.device.GetDepthFormat();
        }
        else
        {
            usage       = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            aspect      = VK_IMAGE_ASPECT_COLOR_BIT;
            imageFormat = ChannelCountToFormat(texture.channelCount, VK_FORMAT_R8G8B8A8_UNORM);
        }

        // Create the VkImage corresponding to this writable texture
        image->Create(&m_context, texture.name, texture.type, texture.width, texture.height, texture.arraySize, imageFormat,
                      VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture.mipLevels, aspect);

        // Increment the generation since we made changes
        texture.generation++;
    }

    void VulkanRendererPlugin::WriteDataToTexture(Texture& texture, u32 offset, const u32 size, const u8* pixels,
                                                  bool includeInFrameWorkload)
    {
        const auto image           = static_cast<VulkanImage*>(texture.internalData);
        const VkFormat imageFormat = ChannelCountToFormat(texture.channelCount, VK_FORMAT_R8G8B8A8_UNORM);

        // Allocate space in our staging buffer
        u64 stagingOffset = 0;
        if (!m_context.stagingBuffer.Allocate(size, stagingOffset))
        {
            ERROR_LOG("Failed to allocate in the staging buffer.");
            return;
        }

        // Load the data into our staging buffer
        if (!m_context.stagingBuffer.LoadRange(stagingOffset, size, pixels, includeInFrameWorkload))
        {
            ERROR_LOG("Failed to load range into staging buffer.");
            return;
        }

        VulkanCommandBuffer tempCommandBuffer;
        VkCommandPool pool = m_context.device.GetGraphicsCommandPool();
        VkQueue queue      = m_context.device.GetGraphicsQueue();

        tempCommandBuffer.AllocateAndBeginSingleUse(&m_context, pool);

        // Transition the layout from whatever it is currently to optimal for receiving data.
        image->TransitionLayout(&tempCommandBuffer, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy the data from the buffer.
        image->CopyFromBuffer(m_context.stagingBuffer.handle, stagingOffset, &tempCommandBuffer);

        if (texture.mipLevels <= 1 || !image->CreateMipMaps(&tempCommandBuffer))
        {
            // If we don't need mips or the generation of the mips fails we fallback to ordinary transition.
            // Transition from optimal for receiving data to shader-read-only optimal layout.
            image->TransitionLayout(&tempCommandBuffer, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        tempCommandBuffer.EndSingleUse(&m_context, pool, queue);

        // Increment the generation since we made changes
        texture.generation++;
    }

    void VulkanRendererPlugin::ResizeTexture(Texture& texture, const u32 newWidth, const u32 newHeight)
    {
        if (texture.internalData)
        {
            const auto image = static_cast<VulkanImage*>(texture.internalData);
            Memory.Delete(image);

            const VkFormat imageFormat = ChannelCountToFormat(texture.channelCount, VK_FORMAT_R8G8B8A8_UNORM);

            // Recalculate our mip levels
            if (texture.mipLevels > 1)
            {
                // Take the base-2 log from the largest dimension floor it and add 1 for the base mip level.
                texture.mipLevels = Floor(Log2(Max(newWidth, newHeight))) + 1;
            }

            // TODO: Lot's of assumptions here
            image->Create(&m_context, texture.name, texture.type, newWidth, newHeight, texture.arraySize, imageFormat,
                          VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, texture.mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);

            // Increment the generation since we have changed the texture
            texture.generation++;
        }
    }

    void VulkanRendererPlugin::ReadDataFromTexture(Texture& texture, const u32 offset, const u32 size, void** outMemory)
    {
        const auto image       = static_cast<VulkanImage*>(texture.internalData);
        const auto imageFormat = ChannelCountToFormat(texture.channelCount, VK_FORMAT_R8G8B8A8_UNORM);

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
        image->TransitionLayout(&tempBuffer, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy the data to the buffer
        image->CopyToBuffer(staging.handle, &tempBuffer);

        // Transition from optimal for data reading to shader-read-only optimal layout
        image->TransitionLayout(&tempBuffer, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tempBuffer.EndSingleUse(&m_context, pool, queue);

        if (!staging.Read(offset, size, outMemory))
        {
            ERROR_LOG("Failed to read from staging buffer.");
        }

        staging.Unbind();
        staging.Destroy();
    }

    void VulkanRendererPlugin::ReadPixelFromTexture(Texture& texture, const u32 x, const u32 y, u8** outRgba)
    {
        const auto image       = static_cast<VulkanImage*>(texture.internalData);
        const auto imageFormat = ChannelCountToFormat(texture.channelCount, VK_FORMAT_R8G8B8A8_UNORM);
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
        image->TransitionLayout(&tempBuffer, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Copy the data to the buffer
        image->CopyPixelToBuffer(staging.handle, x, y, &tempBuffer);

        // Transition from optimal for data reading to shader-read-only optimal layout
        image->TransitionLayout(&tempBuffer, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        tempBuffer.EndSingleUse(&m_context, pool, queue);

        if (!staging.Read(0, size, reinterpret_cast<void**>(outRgba)))
        {
            ERROR_LOG("Failed to read from staging buffer.");
        }

        staging.Unbind();
        staging.Destroy();
    }

    void VulkanRendererPlugin::DestroyTexture(Texture& texture)
    {
        m_context.device.WaitIdle();

        if (const auto image = static_cast<VulkanImage*>(texture.internalData))
        {
            image->Destroy();
            Memory.Delete(texture.internalData);
            texture.internalData = nullptr;
        }
    }

    bool VulkanRendererPlugin::CreateShader(Shader& shader, const ShaderConfig& config, void* pass) const
    {
        // Verify stages
        for (u32 i = 0; i < config.stageConfigs.Size(); i++)
        {
            switch (config.stageConfigs[i].stage)
            {
                case ShaderStage::Fragment:
                    break;
                case ShaderStage::Vertex:
                    break;
                case ShaderStage::Geometry:
                    WARN_LOG("VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.");
                    break;
                case ShaderStage::Compute:
                    WARN_LOG("SHADER_STAGE_COMPUTE is set but not yet supported.");
                    break;
                default:
                    ERROR_LOG("Unknown ShaderStage specified.");
                    break;
            }
        }

        // Allocate the internal vulkan shader
        shader.apiSpecificData = Memory.New<VulkanShader>(MemoryType::Shader, &m_context);
        // Get a pointer to our Vulkan specific shader stuff
        const auto vulkanShader              = static_cast<VulkanShader*>(shader.apiSpecificData);
        vulkanShader->renderpass             = static_cast<VulkanRenderpass*>(pass);
        vulkanShader->localPushConstantBlock = Memory.AllocateBlock(MemoryType::RenderSystem, 128);
        vulkanShader->stageCount             = config.stageConfigs.Size();

        // We need a max of 2 descriptor sets, one for global and one for instance.
        // This is the max so there can also be only 1 or even 0 descriptor sets
        vulkanShader->descriptorSetCount = 0;

        bool hasGlobal   = shader.globalUniformCount > 0 || shader.globalUniformSamplerCount > 0;
        bool hasInstance = shader.instanceUniformCount > 0 || shader.instanceUniformSamplerCount > 0;

        u8 setCount = 0;
        if (hasGlobal)
        {
            vulkanShader->descriptorSets[setCount].samplerBindingIndexStart = INVALID_ID_U8;
            setCount++;
        }
        if (hasInstance)
        {
            vulkanShader->descriptorSets[setCount].samplerBindingIndexStart = INVALID_ID_U8;
            setCount++;
        }

        // Zero out our Attributes array
        std::memset(vulkanShader->attributes, 0, sizeof(VkVertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);

        // Calculate the total number of descriptors that we need
        u32 frameCount = m_context.swapchain.imageCount;
        // 1 set of globals * frameCount + x samplers per instance, per frame
        u32 maxSamplerCount =
            (shader.globalUniformSamplerCount * frameCount) + (config.maxInstances * shader.instanceUniformSamplerCount * frameCount);
        // 1 global (* frameCount) + 1 per instance, per frame
        u32 maxUboCount = frameCount + (config.maxInstances * frameCount);
        // Total number of descriptors needed
        u32 maxDescriptorAllocateCount = maxUboCount + maxSamplerCount;

        vulkanShader->maxDescriptorSetCount = maxDescriptorAllocateCount;
        vulkanShader->maxInstances          = config.maxInstances;

        vulkanShader->descriptorPoolSizeCount = 0;
        if (maxUboCount > 0)
        {
            vulkanShader->descriptorPoolSizes[vulkanShader->descriptorPoolSizeCount] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxUboCount };
            vulkanShader->descriptorPoolSizeCount++;
        }
        if (maxSamplerCount > 0)
        {
            vulkanShader->descriptorPoolSizes[vulkanShader->descriptorPoolSizeCount] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                                         maxSamplerCount };
            vulkanShader->descriptorPoolSizeCount++;
        }

        if (hasGlobal)
        {
            // Configure our global descriptor set config
            auto& setConfig = vulkanShader->descriptorSets[vulkanShader->descriptorSetCount];

            // Total bindings are 1 UBO for global (if needed) plus the global sampler count
            u32 uboCount           = shader.globalUniformCount ? 1 : 0;
            setConfig.bindingCount = uboCount + shader.globalUniformSamplerCount;
            setConfig.bindings.Resize(setConfig.bindingCount);

            // Global UBO binding is first, if present
            u8 globalBindingIndex = 0;
            if (shader.globalUniformCount > 0)
            {
                setConfig.bindings[globalBindingIndex].binding         = globalBindingIndex;
                setConfig.bindings[globalBindingIndex].descriptorCount = 1;  // The whole UBO is one binding
                setConfig.bindings[globalBindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                setConfig.bindings[globalBindingIndex].stageFlags      = VK_SHADER_STAGE_ALL;
                globalBindingIndex++;
            }

            // Set the index where the sampler bindings start. This will be used later to figure out what index to begin binding sampler
            // descriptors at.
            setConfig.samplerBindingIndexStart = shader.globalUniformCount ? 1 : 0;

            // Add a binding for each configured sampler
            for (u32 i = 0; i < shader.globalUniformSamplerCount; ++i)
            {
                const ShaderUniform& u                                 = shader.uniforms[shader.globalSamplers[i]];
                setConfig.bindings[globalBindingIndex].binding         = globalBindingIndex;
                setConfig.bindings[globalBindingIndex].descriptorCount = Max(u.arrayLength, (u8)1);
                setConfig.bindings[globalBindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                setConfig.bindings[globalBindingIndex].stageFlags      = VK_SHADER_STAGE_ALL;
                globalBindingIndex++;
            }

            // Increment our descriptor set counter
            vulkanShader->descriptorSetCount++;
        }

        // If using instance uniforms, add a UBO descriptor set
        if (hasInstance)
        {
            auto& setConfig = vulkanShader->descriptorSets[vulkanShader->descriptorSetCount];

            // Total bindings are 1 UBO for instance (if needed) plus instance sampler count
            u32 uboCount           = shader.instanceUniformCount ? 1 : 0;
            setConfig.bindingCount = uboCount + shader.instanceUniformSamplerCount;
            setConfig.bindings.Resize(setConfig.bindingCount);

            // Instance UBO binding is first if it's present
            u8 instanceBindingIndex = 0;
            if (shader.instanceUniformCount > 0)
            {
                setConfig.bindings[instanceBindingIndex].binding         = instanceBindingIndex;
                setConfig.bindings[instanceBindingIndex].descriptorCount = 1;
                setConfig.bindings[instanceBindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                setConfig.bindings[instanceBindingIndex].stageFlags      = VK_SHADER_STAGE_ALL;
                instanceBindingIndex++;
            }

            // Set the index where the sampler bindings start. This will be used later to figure out what index to begin binding sampler
            // descriptors at.
            setConfig.samplerBindingIndexStart = shader.instanceUniformCount ? 1 : 0;

            // Add a binding for each configured sampler
            for (u32 i = 0; i < shader.instanceUniformSamplerCount; ++i)
            {
                const ShaderUniform& u                                   = shader.uniforms[shader.instanceSamplers[i]];
                setConfig.bindings[instanceBindingIndex].binding         = instanceBindingIndex;
                setConfig.bindings[instanceBindingIndex].descriptorCount = Max(u.arrayLength, (u8)1);
                setConfig.bindings[instanceBindingIndex].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                setConfig.bindings[instanceBindingIndex].stageFlags      = VK_SHADER_STAGE_ALL;
                instanceBindingIndex++;
            }

            // Increment our descriptor set counter
            vulkanShader->descriptorSetCount++;
        }

        // Invalidate all instance states
        vulkanShader->instanceStates.Resize(vulkanShader->maxInstances);
        for (auto& instanceState : vulkanShader->instanceStates)
        {
            instanceState.id = INVALID_ID;
        }

        // Copy over our cull mode
        vulkanShader->cullMode = config.cullMode;
        // Keep a copy of the toplogy types used
        shader.topologyTypes = config.topologyTypes;

        return true;
    }

    bool VulkanRendererPlugin::ReloadShader(Shader& shader) { return CreateShaderModulesAndPipelines(shader); }

    void VulkanRendererPlugin::DestroyShader(Shader& shader)
    {
        // Make sure there is something to destroy
        if (shader.apiSpecificData)
        {
            const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);

            VkDevice logicalDevice                   = m_context.device.GetLogical();
            const VkAllocationCallbacks* vkAllocator = m_context.allocator;

            // Cleanup the descriptor set layouts
            for (u32 i = 0; i < vulkanShader->descriptorSetCount; i++)
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

            // Destroy the instance states
            vulkanShader->instanceStates.Destroy();

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

            for (const auto pipeline : vulkanShader->wireframePipelines)
            {
                if (pipeline)
                {
                    pipeline->Destroy();
                    Memory.Delete(pipeline);
                }
            }

            vulkanShader->pipelines.Destroy();
            vulkanShader->wireframePipelines.Destroy();

            // Cleanup Shader Modules
            for (u32 i = 0; i < vulkanShader->stageCount; i++)
            {
                vkDestroyShaderModule(logicalDevice, vulkanShader->stages[i].handle, vkAllocator);
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

        bool needsWireframe = shader.flags & ShaderFlagWireframe;
        // Determine if we support wireframe and disable if we don't
        if (!m_context.device.SupportsFillmodeNonSolid())
        {
            WARN_LOG("Renderer backend does not support fillModeNonSolid. Wireframe mode will not be available.");
            needsWireframe = false;
        }

        // Static lookup table to convert our internal types to the Vulkan ones.
        static VkFormat* types = nullptr;
        static VkFormat t[10];
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
        u32 offset = 0;
        for (u32 i = 0; i < shader.attributes.Size(); i++)
        {
            // Setup the new attribute
            VkVertexInputAttributeDescription attribute{};
            attribute.location = i;
            attribute.binding  = 0;
            attribute.offset   = offset;
            attribute.format   = types[shader.attributes[i].type];

            vulkanShader->attributes[i] = attribute;
            offset += shader.attributes[i].size;
        }

        // Define the descriptor pool creation info
        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount              = vulkanShader->descriptorPoolSizeCount;
        poolInfo.pPoolSizes                 = vulkanShader->descriptorPoolSizes;
        poolInfo.maxSets                    = vulkanShader->maxDescriptorSetCount;
        poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        // NOTE: increase the per-stage descriptor samplers limit on macOS (maxPerStageDescriptorUpdateAfterBindSamplers >
        // maxPerStageDescriptorSamplers)
        pool_info.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
#endif

        // Create the descriptor pool
        VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, vkAllocator, &vulkanShader->descriptorPool);
        if (!VulkanUtils::IsSuccess(result))
        {
            ERROR_LOG("Failed to create descriptor pool: '{}'.", VulkanUtils::ResultString(result));
            return false;
        }

        // Create descriptor set layouts
        for (u32 i = 0; i < vulkanShader->descriptorSetCount; i++)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            layoutInfo.bindingCount                    = vulkanShader->descriptorSets[i].bindingCount;
            layoutInfo.pBindings                       = vulkanShader->descriptorSets[i].bindings.GetData();

            result = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, vkAllocator, &vulkanShader->descriptorSetLayouts[i]);
            if (!VulkanUtils::IsSuccess(result))
            {
                ERROR_LOG("Failed to create Descriptor Set Layout: '{}'.", VulkanUtils::ResultString(result));
                return false;
            }
        }

        // NOTE: We only support dynamic topology.
        if (m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_STATE) ||
            m_context.device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_STATE))
        {
            // Total of 3 topology classes
            vulkanShader->pipelines.Resize(3);
            // We will also have 3 topology classes for wireframe (if it's supported and requested)
            if (needsWireframe)
            {
                vulkanShader->wireframePipelines.Resize(3);
            }

            // Point class
            if (shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST)
            {
                constexpr auto topologyTypes = PRIMITIVE_TOPOLOGY_TYPE_POINT_LIST;

                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_POINT] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);

                if (needsWireframe)
                {
                    vulkanShader->wireframePipelines[VULKAN_TOPOLOGY_CLASS_POINT] =
                        Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);
                }
            }

            // Line class
            if (shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST || shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP)
            {
                constexpr auto topologyTypes = PRIMITIVE_TOPOLOGY_TYPE_LINE_LIST | PRIMITIVE_TOPOLOGY_TYPE_LINE_STRIP;

                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_LINE] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);

                if (needsWireframe)
                {
                    vulkanShader->wireframePipelines[VULKAN_TOPOLOGY_CLASS_LINE] =
                        Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);
                }
            }

            // Triangle class
            if (shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST ||
                shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP ||
                shader.topologyTypes & PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN)
            {
                constexpr auto topologyTypes =
                    PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_LIST | PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_STRIP | PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE_FAN;

                vulkanShader->pipelines[VULKAN_TOPOLOGY_CLASS_TRIANGLE] = Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);

                if (needsWireframe)
                {
                    vulkanShader->wireframePipelines[VULKAN_TOPOLOGY_CLASS_TRIANGLE] =
                        Memory.New<VulkanPipeline>(MemoryType::Vulkan, topologyTypes);
                }
            }
        }
        else
        {
            // We have no support for dynamic topology. It's probably better if we use a different backend
            FATAL_LOG("Dynamic Topology is not supported by your current setup. Please use a different Rendering backend.");
            return false;
        }

        if (!CreateShaderModulesAndPipelines(shader))
        {
            ERROR_LOG("Failed to create modules and pipelines for: '{}'.", shader.name);
            return false;
        }

        if (vulkanShader->boundPipelineIndex == INVALID_ID_U8)
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
        const u64 totalBufferSize = shader.globalUboStride + (shader.uboStride * vulkanShader->maxInstances);
        if (!vulkanShader->uniformBuffer.Create(RenderBufferType::Uniform, totalBufferSize, RenderBufferTrackType::FreeList))
        {
            ERROR_LOG("Failed to create VulkanBuffer.");
            return false;
        }
        vulkanShader->uniformBuffer.Bind(0);

        // Map the entire buffer's memory
        vulkanShader->mappedUniformBufferBlock = vulkanShader->uniformBuffer.MapMemory(0, VK_WHOLE_SIZE);

        // We only allocate space for the global UBO if needed
        if (shader.globalUboSize > 0 && shader.globalUboStride > 0)
        {
            // Allocate space for the global UBO, which should occupy the stride space and not the actual size needed
            if (!vulkanShader->uniformBuffer.Allocate(shader.globalUboStride, shader.globalUboOffset))
            {
                ERROR_LOG("Failed to allocate space for the uniform buffer.");
                return false;
            }

            // Allocate global descriptor sets, one per frame. Global is always the first set
            // TODO: support other numbers then 3
            const VkDescriptorSetLayout globalLayouts[3] = {
                vulkanShader->descriptorSetLayouts[0],
                vulkanShader->descriptorSetLayouts[0],
                vulkanShader->descriptorSetLayouts[0],
            };

            VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocInfo.descriptorPool              = vulkanShader->descriptorPool;
            allocInfo.descriptorSetCount          = 3;
            allocInfo.pSetLayouts                 = globalLayouts;
            VK_CHECK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, vulkanShader->globalDescriptorSets));

            // Add a debug name to each global descriptor set
            for (u32 i = 0; i < 3; ++i)
            {
                VulkanUtils::SetDebugObjectName(&m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, vulkanShader->globalDescriptorSets[i],
                                                String::FromFormat("{}_GLOBAL_DESCRIPTOR_SET_FRAME_{}", shader.name, i));
            }
        }

        return true;
    }

    bool VulkanRendererPlugin::UseShader(const Shader& shader)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);
        auto commandBuffer      = &m_context.graphicsCommandBuffers[m_context.imageIndex];

        // Pick the correct pipeline
        auto& pipelines = shader.wireframeEnabled ? vulkanShader->wireframePipelines : vulkanShader->pipelines;
        // And then bind it
        pipelines[vulkanShader->boundPipelineIndex]->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
        // Also keep track of the currently bound shader
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

    bool VulkanRendererPlugin::BindShaderLocal(Shader& shader)
    {
        // NOTE: Intentionally left blank since Vulkan does not need to do anything here but other backends might
        return true;
    }

    bool VulkanRendererPlugin::ShaderApplyGlobals(const FrameData& frameData, const Shader& shader, bool needsUpdate)
    {
        // No need to do anything if we have no globals
        if (shader.globalUniformCount == 0 && shader.globalUniformSamplerCount == 0) return true;

        const u32 imageIndex    = m_context.imageIndex;
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);

        VkCommandBuffer commandBuffer       = m_context.graphicsCommandBuffers[imageIndex].handle;
        VkDescriptorSet globalDescriptorSet = vulkanShader->globalDescriptorSets[imageIndex];

        if (needsUpdate)
        {
            VkWriteDescriptorSet descriptorWrites[1 + VULKAN_SHADER_MAX_GLOBAL_TEXTURES];

            u32 descriptorWriteCount = 0;
            u32 bindingIndex         = 0;

            // We only need to update the global UBO if it exists
            if (shader.globalUniformCount > 0)
            {
                // Apply the global UBO first
                VkDescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = vulkanShader->uniformBuffer.handle;
                bufferInfo.offset = shader.globalUboOffset;
                bufferInfo.range  = shader.globalUboStride;

                // Update descriptor sets
                VkWriteDescriptorSet uboWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                uboWrite.dstSet               = vulkanShader->globalDescriptorSets[imageIndex];
                uboWrite.dstBinding           = bindingIndex;
                uboWrite.dstArrayElement      = 0;
                uboWrite.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboWrite.descriptorCount      = 1;
                uboWrite.pBufferInfo          = &bufferInfo;

                descriptorWrites[bindingIndex] = uboWrite;
                descriptorWriteCount++;
                bindingIndex++;
            }

            // We only need to update global samplers if they exist
            if (shader.globalUniformSamplerCount > 0)
            {
                VulkanDescriptorSetConfig setConfig = vulkanShader->descriptorSets[0];

                for (auto& bindingSamplerState : vulkanShader->globalSamplerUniforms)
                {
                    u32 bindingDescriptorCount = setConfig.bindings[bindingIndex].descriptorCount;
                    u32 updateSamplerCount     = 0;
                    VkDescriptorImageInfo imageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];

                    for (u32 d = 0; d < bindingDescriptorCount; ++d)
                    {
                        TextureMap* map             = bindingSamplerState.textureMaps[d];
                        TextureHandle textureHandle = map->texture;
                        // Ensure the texture is valid.
                        if (textureHandle == INVALID_ID)
                        {
                            textureHandle = Textures.GetDefault();
                            // If we are using the default texture, invalidate the maps generation so it's updated next run.
                            map->generation = INVALID_ID;
                        }
                        else
                        {
                            const auto& texture = Textures.Get(textureHandle);

                            if (texture.generation == INVALID_ID)
                            {
                                // generation is always invalid for default textures so let's check if we are using one
                                if (!Textures.IsDefault(textureHandle))
                                {
                                    // If the generation is invalid and we are not using a default texture we set a default one
                                    textureHandle = Textures.GetDefault();
                                }
                                // If we are using the default texture, invalidate the maps generation so it's updated next run.
                                map->generation = INVALID_ID;
                            }
                            else
                            {
                                // If the texture is valid, we ensure that the texture map's generation matches the texture.
                                // If not, the texture map resources should be regenerated
                                if (texture.generation != map->generation)
                                {
                                    bool refreshRequired = texture.mipLevels != map->mipLevels;
                                    if (refreshRequired && !RefreshTextureMapResources(*map))
                                    {
                                        WARN_LOG(
                                            "Failed to refresh texture map resources. This means the sampler settings could be out of "
                                            "date!");
                                    }
                                    else
                                    {
                                        map->generation = texture.generation;
                                    }
                                }
                            }
                        }

                        const auto vulkanTextureData = Textures.GetInternals<VulkanTextureData>(textureHandle);
                        imageInfos[d].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        imageInfos[d].imageView      = vulkanTextureData->image.view;
                        imageInfos[d].sampler        = m_context.samplers[map->internalId];

                        // TODO: Descriptor generations?

                        updateSamplerCount++;
                    }

                    VkWriteDescriptorSet samplerDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    samplerDescriptor.dstSet               = globalDescriptorSet;
                    samplerDescriptor.dstBinding           = bindingIndex;
                    samplerDescriptor.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    samplerDescriptor.descriptorCount      = updateSamplerCount;
                    samplerDescriptor.pImageInfo           = imageInfos;

                    descriptorWrites[descriptorWriteCount] = samplerDescriptor;
                    descriptorWriteCount++;

                    bindingIndex++;
                }
            }

            if (descriptorWriteCount > 0)
            {
                vkUpdateDescriptorSets(m_context.device.GetLogical(), descriptorWriteCount, descriptorWrites, 0, nullptr);
            }
        }

        // Pick the correct pipeline
        auto& pipelines = shader.wireframeEnabled ? vulkanShader->wireframePipelines : vulkanShader->pipelines;

        // Bind the global descriptor set to be updated
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[vulkanShader->boundPipelineIndex]->layout, 0, 1,
                                &globalDescriptorSet, 0, nullptr);
        return true;
    }

    bool VulkanRendererPlugin::ShaderApplyInstance(const FrameData& frameData, const Shader& shader, const bool needsUpdate)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);
        if (shader.instanceUniformCount == 0 && shader.instanceUniformSamplerCount == 0)
        {
            ERROR_LOG("This shader does not use instances.");
            return false;
        }

        const u32 imageIndex          = m_context.imageIndex;
        VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[imageIndex].handle;

        // Obtain instance data
        VulkanShaderInstanceState* instanceState = &vulkanShader->instanceStates[shader.boundInstanceId];
        VkDescriptorSet instanceDescriptorSet    = instanceState->descriptorSets[imageIndex];

        // We only update if it is needed
        if (needsUpdate)
        {
            // Allocate enough descriptor writes to handle the UBO + maximum allowed textures per instance.
            VkWriteDescriptorSet descriptorWrites[1 + VULKAN_SHADER_MAX_INSTANCE_TEXTURES] = {};

            u32 descriptorWriteCount = 0;
            u32 bindingIndex         = 0;

            // Descriptor 0 - Uniform buffer
            if (shader.instanceUniformCount > 0)
            {
                // Only do this if the descriptor has not yet been updated
                u8& instanceUboGeneration = instanceState->uboDescriptorState.generations[imageIndex];
                if (instanceUboGeneration == INVALID_ID_U8)
                {
                    VkDescriptorBufferInfo bufferInfo;
                    bufferInfo.buffer = vulkanShader->uniformBuffer.handle;
                    bufferInfo.offset = instanceState->offset;
                    bufferInfo.range  = shader.uboStride;

                    VkWriteDescriptorSet uboDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    uboDescriptor.dstSet               = instanceDescriptorSet;
                    uboDescriptor.dstBinding           = bindingIndex;
                    uboDescriptor.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    uboDescriptor.descriptorCount      = 1;
                    uboDescriptor.pBufferInfo          = &bufferInfo;

                    descriptorWrites[descriptorWriteCount] = uboDescriptor;
                    descriptorWriteCount++;

                    // Update the frame generation. We only do this once since this is a buffer.
                    instanceUboGeneration = 1;
                }

                bindingIndex++;
            }

            // Iterate samplers.
            if (shader.instanceUniformSamplerCount > 0)
            {
                u8 instanceDescSetIndex             = (shader.globalUniformCount > 0 || shader.globalUniformSamplerCount > 0) ? 1 : 0;
                VulkanDescriptorSetConfig setConfig = vulkanShader->descriptorSets[instanceDescSetIndex];

                for (auto& bindingSamplerState : instanceState->samplerUniforms)
                {
                    u32 bindingDescriptorCount = setConfig.bindings[bindingIndex].descriptorCount;
                    u32 updateSamplerCount     = 0;

                    // Allocate enough imageInfos for every descriptor
                    VkDescriptorImageInfo* imageInfos =
                        frameData.allocator->Allocate<VkDescriptorImageInfo>(MemoryType::RenderSystem, bindingDescriptorCount);

                    for (u32 d = 0; d < bindingDescriptorCount; ++d)
                    {
                        // TODO: only update in the list if actually needing an update
                        TextureMap* map = bindingSamplerState.textureMaps[d];

                        TextureHandle textureHandle = map->texture;
                        // Ensure the texture is valid.
                        if (textureHandle == INVALID_ID)
                        {
                            textureHandle = Textures.GetDefault();
                            // If we are using the default texture, invalidate the maps generation so it's updated next run.
                            map->generation = INVALID_ID;
                        }
                        else
                        {
                            const auto& texture = Textures.Get(textureHandle);
                            if (texture.generation == INVALID_ID)
                            {
                                // Generation is always invalid for default textures so let's check if we are using one
                                if (!Textures.IsDefault(textureHandle))
                                {
                                    // If the generation is invalid and we are not using a default texture we set a default one
                                    if (texture.type == TextureType2D)
                                    {
                                        textureHandle = Textures.GetDefault();
                                    }
                                    else if (texture.type == TextureType2DArray)
                                    {
                                        textureHandle = Textures.GetDefaultTerrain();
                                    }
                                    else if (texture.type == TextureTypeCube)
                                    {
                                        textureHandle = Textures.GetDefaultCube();
                                    }
                                    else
                                    {
                                        ERROR_LOG("Unknown texture type while trying to ApplyInstance. Falling back to default 2D");
                                        textureHandle = Textures.GetDefault();
                                    }
                                }
                                // If we are using the default texture, invalidate the maps generation so it's updated next run.
                                map->generation = INVALID_ID;
                            }
                            else
                            {
                                // If the texture is valid, we ensure that the texture map's generation matches the texture.
                                // If not, the texture map resources should be regenerated
                                if (texture.generation != map->generation)
                                {
                                    bool refreshRequired = texture.mipLevels != map->mipLevels;
                                    if (refreshRequired && !RefreshTextureMapResources(*map))
                                    {
                                        WARN_LOG(
                                            "Failed to refresh texture map resources. This means the sampler settings could be out of "
                                            "date!");
                                    }
                                    else
                                    {
                                        map->generation = texture.generation;
                                    }
                                }
                            }
                        }

                        const auto vulkanTextureData = Textures.GetInternals<VulkanTextureData>(textureHandle);
                        imageInfos[d].imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        imageInfos[d].imageView      = vulkanTextureData->image.view;
                        imageInfos[d].sampler        = m_context.samplers[map->internalId];

                        // TODO: Descriptor generations?

                        updateSamplerCount++;
                    }

                    VkWriteDescriptorSet samplerDescriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    samplerDescriptor.dstSet               = instanceDescriptorSet;
                    samplerDescriptor.dstBinding           = bindingIndex;
                    samplerDescriptor.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    samplerDescriptor.descriptorCount      = updateSamplerCount;
                    samplerDescriptor.pImageInfo           = imageInfos;

                    descriptorWrites[descriptorWriteCount] = samplerDescriptor;
                    descriptorWriteCount++;

                    bindingIndex++;
                }
            }

            if (descriptorWriteCount > 0)
            {
                vkUpdateDescriptorSets(m_context.device.GetLogical(), descriptorWriteCount, descriptorWrites, 0, nullptr);
            }
        }

        // Determine the first descriptor set index. If there are no global this will be 0 otherwise it will be 1
        u32 firstSet = (shader.globalUniformCount > 0 || shader.globalUniformSamplerCount > 0) ? 1 : 0;

        // Pick the correct pipeline
        auto& pipelines = shader.wireframeEnabled ? vulkanShader->wireframePipelines : vulkanShader->pipelines;

        // We always bind for every instance however
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[vulkanShader->boundPipelineIndex]->layout,
                                firstSet, 1, &instanceDescriptorSet, 0, nullptr);
        return true;
    }

    bool VulkanRendererPlugin::ShaderApplyLocal(const FrameData& frameData, const Shader& shader)
    {
        const auto vulkanShader       = static_cast<VulkanShader*>(shader.apiSpecificData);
        VkCommandBuffer commandBuffer = m_context.graphicsCommandBuffers[m_context.imageIndex].handle;

        // Pick the correct pipeline
        auto& pipelines = shader.wireframeEnabled ? vulkanShader->wireframePipelines : vulkanShader->pipelines;

        vkCmdPushConstants(commandBuffer, pipelines[vulkanShader->boundPipelineIndex]->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128, vulkanShader->localPushConstantBlock);

        return true;
    }

    bool VulkanRendererPlugin::ShaderSupportsWireframe(const Shader& shader)
    {
        // If the shader has at least one wireframe pipeline it supports wireframe rendering
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);
        return !vulkanShader->wireframePipelines.Empty();
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

    bool VulkanRendererPlugin::AcquireShaderInstanceResources(const Shader& shader, const ShaderInstanceResourceConfig& config,
                                                              u32& outInstanceId)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);

        outInstanceId = INVALID_ID;
        for (u32 i = 0; i < vulkanShader->maxInstances; i++)
        {
            if (vulkanShader->instanceStates[i].id == INVALID_ID)
            {
                vulkanShader->instanceStates[i].id = i;
                outInstanceId                      = i;
                break;
            }
        }

        if (outInstanceId == INVALID_ID)
        {
            ERROR_LOG("Failed to acquire new instance id.");
            return false;
        }

        TextureHandle defaultTexture = Textures.GetDefault();

        VulkanShaderInstanceState& instanceState = vulkanShader->instanceStates[outInstanceId];
        // Only setup if the shader actually requires it
        if (shader.instanceTextureCount > 0)
        {
            instanceState.samplerUniforms.Resize(shader.instanceUniformSamplerCount);

            // Assign uniforms to each of the sampler states
            for (u32 i = 0; i < shader.instanceUniformSamplerCount; ++i)
            {
                auto& samplerState = instanceState.samplerUniforms[i];
                // Grab a pointer to the uniform associated with this sampler
                samplerState.uniform = &shader.uniforms[shader.instanceSamplers[i]];
                // Grab the uniform texture config also
                const auto& tc = config.uniformConfigs[i];
                // Get the samplers array length (or 1 in case of a single sampler)
                u32 arrayLength = Max(samplerState.uniform->arrayLength, (u8)1);
                // Setup the array for the sampler texture maps
                samplerState.textureMaps.Resize(arrayLength);
                // Setup the descriptor states
                samplerState.descriptorStates.Resize(arrayLength);
                // Per descriptor
                for (u32 d = 0; d < arrayLength; ++d)
                {
                    samplerState.textureMaps[d] = tc.textureMaps[d];
                    // Ensure that we actually have a texture assigned. Use default if we don't
                    if (!samplerState.textureMaps[d]->texture)
                    {
                        samplerState.textureMaps[d]->texture = defaultTexture;
                    }

                    // Per frame
                    // TODO: Handle frameCount != 3
                    for (u32 f = 0; f < 3; ++f)
                    {
                        samplerState.descriptorStates[d].generations[f] = INVALID_ID_U8;
                        samplerState.descriptorStates[d].ids[f]         = INVALID_ID;
                    }
                }
            }
        }

        // Allocate some space in the UBO - by the stride, not the size
        const u64 size = shader.uboStride;
        if (size > 0)
        {
            if (!vulkanShader->uniformBuffer.Allocate(size, instanceState.offset))
            {
                ERROR_LOG("Failed to acquire UBO space.");
                return false;
            }
        }

        // UBO binding. NOTE: really only matters where there are instance uniforms, but set them anyway.
        for (u32 j = 0; j < 3; ++j)
        {
            instanceState.uboDescriptorState.generations[j] = INVALID_ID_U8;
            instanceState.uboDescriptorState.ids[j]         = INVALID_ID_U8;
        }

        u8 instanceDescSetIndex = (shader.globalUniformCount > 0 || shader.globalUniformSamplerCount > 0) ? 1 : 0;
        // Allocate 3 descriptor sets (one per frame)
        // TODO: handle frameCount != 3
        const VkDescriptorSetLayout layouts[3] = {
            vulkanShader->descriptorSetLayouts[instanceDescSetIndex],
            vulkanShader->descriptorSetLayouts[instanceDescSetIndex],
            vulkanShader->descriptorSetLayouts[instanceDescSetIndex],
        };

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool              = vulkanShader->descriptorPool;
        allocInfo.descriptorSetCount          = 3;
        allocInfo.pSetLayouts                 = layouts;
        const VkResult result = vkAllocateDescriptorSets(m_context.device.GetLogical(), &allocInfo, instanceState.descriptorSets);
        if (result != VK_SUCCESS)
        {
            ERROR_LOG("Error allocating descriptor sets in Shader: '{}'.", VulkanUtils::ResultString(result));
            return false;
        }

        // Add a debug name to each global descriptor set
        // TODO: handle frameCount != 3
        for (u32 i = 0; i < 3; ++i)
        {
            VulkanUtils::SetDebugObjectName(&m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, vulkanShader->globalDescriptorSets[i],
                                            String::FromFormat("{}_INSTANCE_DESCRIPTOR_SET_FRAME_{}", shader.name, i));
        }

        return true;
    }

    bool VulkanRendererPlugin::ReleaseShaderInstanceResources(const Shader& shader, const u32 instanceId)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);

        VulkanShaderInstanceState& instanceState = vulkanShader->instanceStates[instanceId];

        // Wait for any pending operations using the descriptor set to finish
        m_context.device.WaitIdle();

        // Free 3 descriptor sets (one per frame)
        // TODO: handle frameCount != 3
        if (vkFreeDescriptorSets(m_context.device.GetLogical(), vulkanShader->descriptorPool, 3, instanceState.descriptorSets) !=
            VK_SUCCESS)
        {
            ERROR_LOG("Error while freeing shader descriptor sets.");
        }

        // Invalidate UBO descriptor states
        for (u32 i = 0; i < 3; i++)
        {
            instanceState.uboDescriptorState.generations[i] = INVALID_ID_U8;
            instanceState.uboDescriptorState.ids[i]         = INVALID_ID;
        }

        // Destroy bindings and their descriptor states/uniforms
        for (auto& samplerState : instanceState.samplerUniforms)
        {
            samplerState.descriptorStates.Destroy();
            samplerState.textureMaps.Destroy();
        }

        if (shader.uboStride != 0)
        {
            if (!vulkanShader->uniformBuffer.Free(shader.uboStride, instanceState.offset))
            {
                ERROR_LOG("Failed to free range from renderbuffer.");
            }
        }

        instanceState.offset = INVALID_ID_U64;
        instanceState.id     = INVALID_ID;

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

        const auto samplerName = Textures.GetName(map.texture) + "_texture_map_sampler";
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

    static bool TrySetSamplerState(DynamicArray<VulkanUniformSamplerState>& samplerUniforms, u32 samplerCount, u16 uniformLocation,
                                   u32 arrayIndex, TextureMap* map)
    {
        // Find the correct sampler uniform state to update
        for (auto& state : samplerUniforms)
        {
            if (state.uniform->location == uniformLocation)
            {
                if (state.uniform->arrayLength > 1)
                {
                    if (arrayIndex >= state.uniform->arrayLength)
                    {
                        ERROR_LOG("ArrayIndex of: {} is out of range (0-{}).", arrayIndex, state.uniform->arrayLength);
                        return false;
                    }
                    state.textureMaps[arrayIndex] = map;
                }
                else
                {
                    state.textureMaps[0] = map;
                }

                return true;
            }
        }

        ERROR_LOG("Unable to find uniform location: {}. Sampler uniform was not set.", uniformLocation);
        return false;
    }

    bool VulkanRendererPlugin::SetUniform(Shader& shader, const ShaderUniform& uniform, u32 arrayIndex, const void* value)
    {
        const auto vulkanShader = static_cast<VulkanShader*>(shader.apiSpecificData);
        if (UniformTypeIsASampler(uniform.type))
        {
            // Samplers can only be assigned at global or instance level
            auto map = (TextureMap*)value;

            if (uniform.scope == ShaderScope::Global)
            {
                return TrySetSamplerState(vulkanShader->globalSamplerUniforms, shader.globalUniformSamplerCount, uniform.location,
                                          arrayIndex, map);
            }
            else
            {
                VulkanShaderInstanceState& instanceState = vulkanShader->instanceStates[shader.boundInstanceId];
                return TrySetSamplerState(instanceState.samplerUniforms, shader.instanceUniformSamplerCount, uniform.location, arrayIndex,
                                          map);
            }
        }
        else
        {
            // Not a sampler so this is a regular uniform
            u8* address = nullptr;

            if (uniform.scope == ShaderScope::Local)
            {
                // The uniform is local so we use push constants
                address = static_cast<u8*>(vulkanShader->localPushConstantBlock);
                address += uniform.offset + (uniform.size * arrayIndex);
            }
            else
            {
                // Map the appropriate memory location and copy the data over.
                address = static_cast<u8*>(vulkanShader->mappedUniformBufferBlock);
                address += shader.boundUboOffset + uniform.offset + (uniform.size * arrayIndex);
            }

            std::memcpy(address, value, uniform.size);
        }

        return true;
    }

    void VulkanRendererPlugin::CreateCommandBuffers()
    {
        if (m_context.graphicsCommandBuffers.Empty())
        {
            m_context.graphicsCommandBuffers.Resize(m_context.swapchain.imageCount);
        }

        auto graphicsCommandPool = m_context.device.GetGraphicsCommandPool();
        for (u32 i = 0; i < m_context.swapchain.imageCount; i++)
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
        for (u32 i = 0; i < m_context.swapchain.imageCount; i++)
        {
            m_context.imagesInFlight[i] = nullptr;
        }

        // Re-query the SwapChain support and depth format since it might have changed
        m_context.device.QuerySwapChainSupport();
        m_context.device.DetectDepthFormat();

        m_context.swapchain.Recreate(m_context.frameBufferWidth, m_context.frameBufferHeight, m_config.flags);

        // Update the size generation so that they are in sync again
        m_context.frameBufferSizeLastGeneration = m_context.frameBufferSizeGeneration;

        // Cleanup SwapChain
        auto graphicsCommandPool = m_context.device.GetGraphicsCommandPool();
        for (u32 i = 0; i < m_context.swapchain.imageCount; i++)
        {
            m_context.graphicsCommandBuffers[i].Free(&m_context, graphicsCommandPool);
        }

        // Tell the renderer that a refresh is required
        Event.Fire(EventCodeDefaultRenderTargetRefreshRequired, nullptr, {});

        CreateCommandBuffers();

        m_context.recreatingSwapChain = false;
        return true;
    }

    bool VulkanRendererPlugin::CreateShaderModulesAndPipelines(Shader& shader)
    {
        constexpr auto VULKAN_SHADER_NUM_PIPELINES = 3;

        auto vulkanShader   = static_cast<VulkanShader*>(shader.apiSpecificData);
        bool needsWireframe = !vulkanShader->wireframePipelines.Empty();

        VulkanShaderStage newStages[VULKAN_SHADER_MAX_STAGES];
        VulkanPipeline* newPipelines[VULKAN_SHADER_NUM_PIPELINES];
        VulkanPipeline* newWireframePipelines[VULKAN_SHADER_NUM_PIPELINES];

        for (u32 i = 0; i < vulkanShader->stageCount; i++)
        {
            if (!CreateShaderModule(shader.stageConfigs[i], newStages[i]))
            {
                ERROR_LOG("Unable to create: '{}' shader module for: '{}'. Stopping Shader creation.", shader.stageConfigs[i].fileName,
                          shader.name);
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
        for (u32 i = 0; i < vulkanShader->stageCount; i++)
        {
            stageCreateInfos[i] = newStages[i].shaderStageCreateInfo;
        }

        bool inError = false;
        for (u32 i = 0; i < VULKAN_TOPOLOGY_CLASS_MAX; i++)
        {
            // Check if there is currently a pipeline there
            if (!vulkanShader->pipelines[i]) continue;

            VulkanPipelineConfig config = {};
            config.renderpass           = vulkanShader->renderpass;
            config.stride               = shader.attributeStride;
            config.attributes.Copy(vulkanShader->attributes, shader.attributes.Size());
            config.descriptorSetLayouts.Copy(vulkanShader->descriptorSetLayouts, vulkanShader->descriptorSetCount);
            config.stages.Copy(stageCreateInfos, vulkanShader->stageCount);
            config.viewport = viewport;
            config.scissor  = scissor;
            config.cullMode = vulkanShader->cullMode;
            // For the non-wireframe shader we pass flags without wireframe
            config.shaderFlags = (shader.flags & ~ShaderFlagWireframe);
            config.pushConstantRanges.Resize(1);
            Range pushConstantRange;
            pushConstantRange.offset     = 0;
            pushConstantRange.size       = shader.localUboStride;
            config.pushConstantRanges[0] = pushConstantRange;
            config.shaderName            = shader.name;
            config.topologyTypes         = shader.topologyTypes;

            if (vulkanShader->boundPipelineIndex == INVALID_ID_U8)
            {
                // Set the bound pipeline to the first valid pipeline
                vulkanShader->boundPipelineIndex = i;
            }

            newPipelines[i] = Memory.New<VulkanPipeline>(MemoryType::RenderSystem, vulkanShader->pipelines[i]->GetSupportedTopologyTypes());
            if (!newPipelines[i]->Create(&m_context, config))
            {
                ERROR_LOG("Failed to create pipeline for topology type: '{}'.", i);
                inError = true;
                break;
            }

            if (needsWireframe)
            {
                // If we need the wireframe pipeline for this topology type then we simply use the same config but with the wireframe flag
                newWireframePipelines[i] =
                    Memory.New<VulkanPipeline>(MemoryType::RenderSystem, vulkanShader->wireframePipelines[i]->GetSupportedTopologyTypes());

                config.shaderFlags = shader.flags;
                if (!newWireframePipelines[i]->Create(&m_context, config))
                {
                    ERROR_LOG("Failed to create wireframe pipeline for topology type: '{}'.", i);
                    inError = true;
                    break;
                }
            }
        }

        if (inError)
        {
            ERROR_LOG("Failed to Create Shader Modules or Pipelines for: '{}'.", shader.name);
            ERROR_LOG("Deleting newly created pipelines.");
            for (u32 i = 0; i < vulkanShader->pipelines.Size(); ++i)
            {
                if (newPipelines[i])
                {
                    newPipelines[i]->Destroy();
                    Memory.Delete(newPipelines[i]);
                }

                if (newWireframePipelines[i])
                {
                    newWireframePipelines[i]->Destroy();
                    Memory.Delete(newWireframePipelines[i]);
                }
            }

            ERROR_LOG("Deleting newly created modules.");
            for (auto& stage : newStages)
            {
                vkDestroyShaderModule(m_context.device.GetLogical(), stage.handle, m_context.allocator);
            }

            return false;
        }

        // We succesfully created our new modules and pipelines so let's destroy the old ones and replace them.
        INFO_LOG("Replacing old pipelines with newly generated ones for: '{}'.", shader.name);
        m_context.device.WaitIdle();
        for (u32 i = 0; i < vulkanShader->pipelines.Size(); ++i)
        {
            if (vulkanShader->pipelines[i])
            {
                vulkanShader->pipelines[i]->Destroy();
                Memory.Delete(vulkanShader->pipelines[i]);

                vulkanShader->pipelines[i] = newPipelines[i];

                if (needsWireframe)
                {
                    vulkanShader->wireframePipelines[i]->Destroy();
                    Memory.Delete(vulkanShader->wireframePipelines[i]);

                    vulkanShader->wireframePipelines[i] = newWireframePipelines[i];
                }
            }
        }

        INFO_LOG("Replacing old modules with newly generated ones for: '{}'.", shader.name);
        for (u32 i = 0; i < vulkanShader->stageCount; ++i)
        {
            vkDestroyShaderModule(m_context.device.GetLogical(), vulkanShader->stages[i].handle, m_context.allocator);
            vulkanShader->stages[i] = newStages[i];
        }

        return true;
    }

    bool VulkanRendererPlugin::CreateShaderModule(const ShaderStageConfig& config, VulkanShaderStage& outStage) const
    {
        shaderc_shader_kind shaderKind;
        VkShaderStageFlagBits stage;
        switch (config.stage)
        {
            case ShaderStage::Vertex:
                shaderKind = shaderc_glsl_default_vertex_shader;
                stage      = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStage::Fragment:
                shaderKind = shaderc_glsl_default_fragment_shader;
                stage      = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderStage::Compute:
                shaderKind = shaderc_glsl_default_compute_shader;
                stage      = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            case ShaderStage::Geometry:
                shaderKind = shaderc_glsl_default_geometry_shader;
                stage      = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            default:
                ERROR_LOG("Unsupported shader kind. Unable to create ShaderModule.");
                return false;
        }

        INFO_LOG("Compiling: '{}' Stage for ShaderModule: '{}'.", ToString(stage), config.name);

        // Attempt to compile the shader
        shaderc_compilation_result_t compilationResult = shaderc_compile_into_spv(
            m_context.shaderCompiler, config.source.Data(), config.source.Size(), shaderKind, config.fileName.Data(), "main", nullptr);

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

        INFO_LOG("Successfully compiled: '{}' Stage consisting of {} bytes for ShaderModule: '{}'.", ToString(stage), byteCount,
                 config.fileName);

        outStage.createInfo          = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        outStage.createInfo.codeSize = byteCount;
        outStage.createInfo.pCode    = code;

        VK_CHECK(vkCreateShaderModule(m_context.device.GetLogical(), &outStage.createInfo, m_context.allocator, &outStage.handle));

        // Release our allocated memory again since the ShaderModule has been created
        Memory.Free(code);

        VK_SET_DEBUG_OBJECT_NAME(&m_context, VK_OBJECT_TYPE_SHADER_MODULE, outStage.handle, config.name);

        // Shader stage info
        outStage.shaderStageCreateInfo        = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        outStage.shaderStageCreateInfo.stage  = stage;
        outStage.shaderStageCreateInfo.module = outStage.handle;
        // TODO: make this configurable?
        outStage.shaderStageCreateInfo.pName = "main";

        return true;
    }

    VkSampler VulkanRendererPlugin::CreateSampler(TextureMap& map)
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

        const auto& texture = Textures.Get(map.texture);

        // Sync mip levels between texture and texturemap
        map.mipLevels = texture.mipLevels;

        samplerInfo.minFilter = ConvertFilterType("min", map.minifyFilter);
        samplerInfo.magFilter = ConvertFilterType("mag", map.magnifyFilter);

        samplerInfo.addressModeU = ConvertRepeatType("U", map.repeatU);
        samplerInfo.addressModeV = ConvertRepeatType("V", map.repeatV);
        samplerInfo.addressModeW = ConvertRepeatType("W", map.repeatW);

        // TODO: Configurable
        samplerInfo.anisotropyEnable        = VK_TRUE;
        samplerInfo.maxAnisotropy           = 16;
        samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias              = 0.0f;
        // Use the full range of mips available
        samplerInfo.minLod = 0.0f;
        // samplerInfo.minLod = map.texture->mipLevels > 1 ? map.texture->mipLevels : 0.0f;
        samplerInfo.maxLod = texture.mipLevels;

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
