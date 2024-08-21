
#include "vulkan_swapchain.h"

#include <core/engine.h>
#include <core/logger.h>
#include <math/c3d_math.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include "vulkan_device.h"
#include "vulkan_image.h"
#include "vulkan_types.h"

namespace C3D
{
    VkSurfaceFormatKHR VulkanSwapchain::GetSurfaceFormat() const
    {
        const auto& formats = m_context->device.GetSurfaceFormats();
        for (auto& format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }

        WARN_LOG("Could not find Preferred SwapChain ImageFormat. Falling back to first format in the list.");
        return formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::GetPresentMode() const
    {
        // VSync is enabled
        if (m_flags & FlagVSyncEnabled)
        {
            if (!(m_flags & FlagPowerSavingEnabled))
            {
                // If power saving is not enabled we can try to see if Mailbox is supported
                // this will generate frames as fast as it can (which is less power efficient but it reduces input lag)
                const auto& presentModes = m_context->device.GetPresentModes();
                for (auto presentMode : presentModes)
                {
                    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        return presentMode;
                    }
                }
            }

            // If power saving is enabled or mailbox is not supported we fallback to FIFO
            // which is guaranteed to be supported by all GPUs
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        // We use immediate mode if VSync is disabled which will render as many fps as possible
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    void VulkanSwapchain::Create(const VulkanContext* context, const u32 width, const u32 height, const RendererConfigFlags flags)
    {
        m_context = context;

        CreateInternal(width, height, flags);
    }

    void VulkanSwapchain::Recreate(const u32 width, const u32 height, const RendererConfigFlags flags)
    {
        DestroyInternal();
        CreateInternal(width, height, flags);
    }

    void VulkanSwapchain::Destroy()
    {
        INFO_LOG("Destroying SwapChain.");
        DestroyInternal();

        // Since we don't destroy our depth and render textures in destroy internal (so we can re-use the textures on a
        // recreate() call) We still need to cleanup our depth textures
        Memory.Delete(depthTextures);
        depthTextures = nullptr;

        // And we also need to cleanup our render textures
        for (u32 i = 0; i < imageCount; i++)
        {
            // We start with the vulkan internal data
            const auto img = static_cast<VulkanImage*>(renderTextures[i].internalData);
            Memory.Delete(img);
            // Cleanup the render texture names
            renderTextures[i].name.Destroy();
        }

        // then we cleanup the actual render textures themselves
        Memory.Delete(renderTextures);
        renderTextures = nullptr;
    }

    bool VulkanSwapchain::AcquireNextImageIndex(const u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex)
    {
        const auto result =
            vkAcquireNextImageKHR(m_context->device.GetLogical(), handle, timeoutNs, imageAvailableSemaphore, fence, outImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            Recreate(m_context->frameBufferWidth, m_context->frameBufferHeight, m_flags);
            return false;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            FATAL_LOG("Failed to acquire SwapChain image.");
            return false;
        }
        return true;
    }

    void VulkanSwapchain::Present(VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, const u32 presentImageIndex)
    {
        // Return the image to the SwapChain for presentation
        VkPresentInfoKHR presentInfo   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &renderCompleteSemaphore;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &handle;
        presentInfo.pImageIndices      = &presentImageIndex;
        presentInfo.pResults           = nullptr;

        // HACK: Waiting here for the present queue prevents us from crashing the application
        // after it has been running for a while even though we are using the present queue here...
        // vkQueueWaitIdle(m_context->device.GetTransferQueue());

        const auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            // Our SwapChain is out of date, suboptimal or a FrameBuffer resize has occurred.
            // We trigger a SwapChain recreation.
            Recreate(m_context->frameBufferWidth, m_context->frameBufferHeight, m_flags);
            DEBUG_LOG("Recreated because SwapChain returned out of date or suboptimal.");
        }
        else if (result != VK_SUCCESS)
        {
            FATAL_LOG("Failed to present SwapChain image.");
        }

        m_context->currentFrame = (m_context->currentFrame + 1) % maxFramesInFlight;
    }

    void VulkanSwapchain::CreateInternal(const u32 width, const u32 height, const RendererConfigFlags flags)
    {
        VkExtent2D extent = { width, height };
        m_flags           = flags;
        imageFormat       = GetSurfaceFormat();
        m_presentMode     = GetPresentMode();

        // Query SwapChain support again to see if anything changed since last time (for example different resolution or
        // monitor)
        m_context->device.QuerySwapChainSupport();

        const auto& capabilities = m_context->device.GetSurfaceCapabilities();
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            extent = capabilities.currentExtent;
        }

        const VkExtent2D min = capabilities.minImageExtent;
        const VkExtent2D max = capabilities.maxImageExtent;

        extent.width  = Clamp(extent.width, min.width, max.width);
        extent.height = Clamp(extent.height, min.height, max.height);

        u32 imgCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imgCount > capabilities.maxImageCount)
        {
            imgCount = capabilities.maxImageCount;
        }

        maxFramesInFlight = static_cast<u8>(imgCount) - 1;

        VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapChainCreateInfo.surface                  = m_context->surface;
        swapChainCreateInfo.minImageCount            = imgCount;
        swapChainCreateInfo.imageFormat              = imageFormat.format;
        swapChainCreateInfo.imageColorSpace          = imageFormat.colorSpace;
        swapChainCreateInfo.imageExtent              = extent;
        swapChainCreateInfo.imageArrayLayers         = 1;
        swapChainCreateInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto graphicsQueueIndex = m_context->device.GetGraphicsQueueIndex();
        auto presentQueueIndex  = m_context->device.GetPresentQueueIndex();

        if (graphicsQueueIndex != presentQueueIndex)
        {
            const u32 queueFamilyIndices[] = { graphicsQueueIndex, presentQueueIndex };

            swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0;
            swapChainCreateInfo.pQueueFamilyIndices   = nullptr;
        }

        swapChainCreateInfo.preTransform   = capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.presentMode    = m_presentMode;
        swapChainCreateInfo.clipped        = VK_TRUE;
        // TODO: pass the old SwapChain here for better performance
        swapChainCreateInfo.oldSwapchain = nullptr;

        auto logicalDevice = m_context->device.GetLogical();

        VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, m_context->allocator, &handle));

        m_context->currentFrame = 0;

        imageCount = 0;
        VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, nullptr));
        if (!renderTextures)
        {
            renderTextures = Memory.NewArray<Texture>(MemoryType::Texture, imageCount);
            // If creating the array, then the internal texture objects aren't created yet either.
            for (u32 i = 0; i < imageCount; ++i)
            {
                const auto internalData = Memory.New<VulkanImage>(MemoryType::Texture);

                char texName[38] = "__internal_vulkan_swapChain_image_0__";
                texName[34]      = '0' + static_cast<char>(i);

                renderTextures[i] = Textures.WrapInternal(texName, extent.width, extent.height, 4, internalData);

                if (!renderTextures[i].internalData)
                {
                    FATAL_LOG("Failed to generate new swapChain image texture.");
                    return;
                }
            }
        }
        else
        {
            for (u32 i = 0; i < imageCount; ++i)
            {
                // Just update the dimensions.
                Textures.Resize(renderTextures[i], extent.width, extent.height, false);
            }
        }

        VkImage swapChainImages[32];
        VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, handle, &imageCount, swapChainImages));
        for (u32 i = 0; i < imageCount; i++)
        {
            // Update the internal image for each.
            const auto image = static_cast<VulkanImage*>(renderTextures[i].internalData);
            image->handle    = swapChainImages[i];
            image->width     = extent.width;
            image->height    = extent.height;
        }

        // Views
        for (u32 i = 0; i < imageCount; i++)
        {
            const auto image = static_cast<VulkanImage*>(renderTextures[i].internalData);

            VkImageViewCreateInfo viewInfo           = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewInfo.image                           = image->handle;
            viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                          = imageFormat.format;
            viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(logicalDevice, &viewInfo, m_context->allocator, &image->view));
        }

        // Detect depth resources
        if (!m_context->device.DetectDepthFormat())
        {
            FATAL_LOG("Failed to find a supported Depth Format.");
        }

        // If we do not have an array for our depth textures yet we allocate it
        if (!depthTextures)
        {
            depthTextures = Memory.NewArray<Texture>(MemoryType::Texture, imageCount);
        }

        for (u32 i = 0; i < imageCount; i++)
        {
            // Create a depth/stencil image and it's view
            const auto name  = String::FromFormat("__C3D_DEFAULT_DEPTH_STENCIL_TEXTURE_{}", i);
            const auto image = Memory.New<VulkanImage>(MemoryType::Texture);
            image->Create(m_context, name, TextureType2D, extent.width, extent.height, 1, m_context->device.GetDepthFormat(),
                          VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true,
                          1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

            // Wrap it in a texture
            depthTextures[i] =
                Textures.WrapInternal(name.Data(), extent.width, extent.height, m_context->device.GetDepthChannelCount(), image);
        }

        INFO_LOG("Successfully created.");
    }

    void VulkanSwapchain::DestroyInternal() const
    {
        m_context->device.WaitIdle();

        // Destroy our internal depth textures
        for (u32 i = 0; i < imageCount; i++)
        {
            // First we let the texture system know that we are removing this texture
            Textures.ReleaseInternal(depthTextures[i]);

            // First we destroy the internal vulkan specific data for every depth texture
            const auto image = static_cast<VulkanImage*>(depthTextures[i].internalData);
            Memory.Delete(image);

            depthTextures[i].internalData = nullptr;
            depthTextures[i].name.Destroy();
        }

        auto logicalDevice = m_context->device.GetLogical();

        // Destroy our views for our render textures
        for (u32 i = 0; i < imageCount; i++)
        {
            // First we destroy the internal vulkan specific data for every render texture
            const auto image = static_cast<VulkanImage*>(renderTextures[i].internalData);
            vkDestroyImageView(logicalDevice, image->view, m_context->allocator);
        }

        vkDestroySwapchainKHR(logicalDevice, handle, m_context->allocator);
    }
}  // namespace C3D
