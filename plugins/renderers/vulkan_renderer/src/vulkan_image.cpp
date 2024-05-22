
#include "vulkan_image.h"

#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <platform/platform.h>
#include <systems/system_manager.h>

#include "vulkan_utils.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "VULKAN_IMAGE";

    VulkanImage::~VulkanImage() { Destroy(); }

    constexpr VkImageType GetVkImageType(const TextureType type)
    {
        /* TODO: This currently is not used
        switch (type)
        {
            default:
            case TextureType2D:
            case TextureTypeCube:
            case TextureType2DArray:
                return VK_IMAGE_TYPE_2D;
        }
        */
        return VK_IMAGE_TYPE_2D;
    }

    constexpr VkImageViewType GetVkImageViewType(const TextureType type)
    {
        switch (type)
        {
            case TextureType2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType2DArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case TextureTypeCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureTypeCubeArray:
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            default:
                FATAL_LOG("Invalid TextureType provided: {}.", ToString(type));
                return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    bool VulkanImage::Create(const VulkanContext* context, const String& name, const TextureType type, const u32 w, const u32 h,
                             u16 layerCount, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage,
                             const VkMemoryPropertyFlags memoryFlags, const bool createView, u8 mipLevels,
                             const VkImageAspectFlags viewAspectFlags)
    {
        m_context     = context;
        m_name        = name;
        width         = w;
        height        = h;
        m_format      = format;
        m_layerCount  = Max(layerCount, (u16)1);
        m_memoryFlags = memoryFlags;
        m_mipLevels   = mipLevels;

        if (m_mipLevels < 1)
        {
            WARN_LOG("MipLevels must be >= 1 for: '{}'. Defaulting to 1.", m_name);
            m_mipLevels = 1;
        }

        VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCreateInfo.imageType         = GetVkImageType(type);
        imageCreateInfo.extent.width      = width;
        imageCreateInfo.extent.height     = height;
        // TODO: Support different depth.
        imageCreateInfo.extent.depth  = 1;
        imageCreateInfo.mipLevels     = m_mipLevels;
        imageCreateInfo.arrayLayers   = m_layerCount;
        imageCreateInfo.format        = format;
        imageCreateInfo.tiling        = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage         = usage;
        // TODO: Configurable sample count.
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        // TODO: Configurable sharing mode.
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (type == TextureTypeCube)
        {
            imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        auto logicalDevice = context->device.GetLogical();

        VK_CHECK(vkCreateImage(logicalDevice, &imageCreateInfo, context->allocator, &handle));

        VK_SET_DEBUG_OBJECT_NAME(context, VK_OBJECT_TYPE_IMAGE, handle, m_name);

        vkGetImageMemoryRequirements(logicalDevice, handle, &m_memoryRequirements);

        const i32 memoryType = context->device.FindMemoryIndex(m_memoryRequirements.memoryTypeBits, memoryFlags);
        if (memoryType == -1)
        {
            ERROR_LOG("Required memory type not found for: '{}'. Image not valid.", m_name);
            return false;
        }

        // Allocate memory
        VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize       = m_memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex      = memoryType;
        VK_CHECK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, context->allocator, &m_memory));

        VK_SET_DEBUG_OBJECT_NAME(context, VK_OBJECT_TYPE_DEVICE_MEMORY, m_memory, m_name);

        // Bind the memory
        // TODO: configurable memory offset
        VK_CHECK(vkBindImageMemory(logicalDevice, handle, m_memory, 0));

        // Determine if memory is device local (on the GPU)
        const bool isDeviceMemory = m_memoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto size           = m_memoryRequirements.size;
        // Report memory as in-use
        MetricsAllocate(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, size, size, m_memory);

        if (createView)
        {
            view = CreateView(type, layerCount, -1, viewAspectFlags);

            if (layerCount > 1)
            {
                // Multiple views, one per layer
                layerViews.Resize(layerCount);

                TextureType viewType = type;
                if (type == TextureTypeCube || type == TextureTypeCubeArray)
                {
                    // NOTE: For sampling of individual array layers for cube or cube array texture our type should be 2D
                    viewType = TextureType2D;
                }

                for (u32 i = 0; i < layerCount; ++i)
                {
                    layerViews[i] = CreateView(viewType, 1, i, viewAspectFlags);
                }
            }
        }

        return true;
    }

    VkImageView VulkanImage::CreateView(TextureType type, u16 layerCount, i32 layerIndex, VkImageAspectFlags aspectFlags)
    {
        VkImageView v = nullptr;

        VkImageViewCreateInfo viewCreateInfo       = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCreateInfo.image                       = handle;
        viewCreateInfo.viewType                    = GetVkImageViewType(type);
        viewCreateInfo.format                      = m_format;
        viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

        viewCreateInfo.subresourceRange.baseMipLevel   = 0;
        viewCreateInfo.subresourceRange.levelCount     = m_mipLevels;
        viewCreateInfo.subresourceRange.layerCount     = layerIndex < 0 ? layerCount : 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = layerIndex < 0 ? 0 : layerIndex;

        VK_CHECK(vkCreateImageView(m_context->device.GetLogical(), &viewCreateInfo, m_context->allocator, &v));

        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_IMAGE_VIEW, v, String::FromFormat("{}_IMAGE_VIEW_{}", m_name, layerIndex));

        return v;
    }

    bool VulkanImage::CreateMipMaps(const VulkanCommandBuffer* commandBuffer)
    {
        if (m_mipLevels <= 1)
        {
            WARN_LOG("Attempted to create mips for image: '{}', that only requires 1 mip level.", m_name);
            return true;
        }

        // Ensure the image format support linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_context->device.GetPhysical(), m_format, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            WARN_LOG("Texture image format for image: '{}', does not support linear blitting! Mipmaps can't be created.", m_name);
            return false;
        }

        VkImageMemoryBarrier barrier            = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image                           = handle;
        barrier.srcQueueFamilyIndex             = m_context->device.GetGraphicsQueueIndex();
        barrier.dstQueueFamilyIndex             = m_context->device.GetGraphicsQueueIndex();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;

        // One mip level at the time
        barrier.subresourceRange.levelCount = 1;

        // Generate for all layers
        barrier.subresourceRange.layerCount = m_layerCount;

        i32 mipWidth  = width;
        i32 mipHeight = height;

        // Iterate each sub-mip level, starting at 1 (since we can skip the original full res image).
        // Each mip level uses the previous level as source material for the blitting operation
        for (u32 i = 1; i < m_mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

            // Transition the mip image subresource to a transfer layout.
            vkCmdPipelineBarrier(commandBuffer->handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                                 nullptr, 1, &barrier);

            // Setup the blit
            VkImageBlit blit = {};
            // Source offset is always the upper-left corner
            blit.srcOffsets[0] = { 0, 0, 0 };
            // The extents of our source mip level
            blit.srcOffsets[1]             = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // Source is the previous mip level
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = m_layerCount;
            // Destination offset is also always the upper-left corner
            blit.dstOffsets[0] = { 0, 0, 0 };
            // Next mips width and height is half the current mip width/height (unless current == 1)
            i32 nextMipWidth               = mipWidth > 1 ? mipWidth / 2 : 1;
            i32 nextMipHeight              = mipHeight > 1 ? mipHeight / 2 : 1;
            blit.dstOffsets[1]             = { nextMipWidth, nextMipHeight, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // The destination is the current mip level
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = m_layerCount;

            // Perform a blit for this layer
            vkCmdBlitImage(commandBuffer->handle, handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // Transition the previous mip layer's image subresource to a shader-readable layout
            vkCmdPipelineBarrier(commandBuffer->handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);

            // Now our current width/height needs to be updated to the next width/height
            mipWidth  = nextMipWidth;
            mipHeight = nextMipHeight;
        }

        // Finally transition the last mipmap level to a shader-readable layout.
        barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer->handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        return true;
    }

    void VulkanImage::TransitionLayout(const VulkanCommandBuffer* commandBuffer, VkFormat format, const VkImageLayout oldLayout,
                                       const VkImageLayout newLayout) const
    {
        auto graphicsQueueIndex = m_context->device.GetGraphicsQueueIndex();

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destStage;

        VkImageMemoryBarrier barrier        = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout                   = oldLayout;
        barrier.newLayout                   = newLayout;
        barrier.srcQueueFamilyIndex         = graphicsQueueIndex;
        barrier.dstQueueFamilyIndex         = graphicsQueueIndex;
        barrier.image                       = handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Mips
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount   = m_mipLevels;

        // Transition all layers at once
        barrier.subresourceRange.layerCount = m_layerCount;

        // Start at the first layer
        barrier.subresourceRange.baseArrayLayer = 0;

        // TODO: only set source/dest stage once...
        // Don't care about the old layout - transfer to optimal layout for the GPU's underlying implementation
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            // Don't care what stage the pipeline is in at the start
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            // Used for copying
            destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        // Transition from a transfer destination to a shader-readonly layout
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // From a copying stage to
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            // The fragment stage
            destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destStage   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destStage   = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
        {
            FATAL_LOG("Unsupported layout transition for: '{}'.", m_name);
            return;
        }

        vkCmdPipelineBarrier(commandBuffer->handle, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VulkanImage::CopyFromBuffer(VkBuffer buffer, u64 offset, const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = offset;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = m_layerCount;

        region.imageExtent.width  = width;
        region.imageExtent.height = height;
        region.imageExtent.depth  = 1;

        vkCmdCopyBufferToImage(commandBuffer->handle, buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void VulkanImage::CopyToBuffer(VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = m_layerCount;

        region.imageExtent.width  = width;
        region.imageExtent.height = height;
        region.imageExtent.depth  = 1;

        vkCmdCopyImageToBuffer(commandBuffer->handle, handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
    }

    void VulkanImage::CopyPixelToBuffer(VkBuffer buffer, const u32 x, const u32 y, const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = m_layerCount;

        region.imageOffset.x      = static_cast<i32>(x);
        region.imageOffset.y      = static_cast<i32>(y);
        region.imageExtent.width  = 1;
        region.imageExtent.height = 1;
        region.imageExtent.depth  = 1;

        vkCmdCopyImageToBuffer(commandBuffer->handle, handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
    }

    void VulkanImage::Destroy()
    {
        if (m_context)
        {
            auto logicalDevice = m_context->device.GetLogical();

            if (view)
            {
                vkDestroyImageView(logicalDevice, view, m_context->allocator);
                view = nullptr;
            }

            if (!layerViews.Empty())
            {
                for (auto& v : layerViews)
                {
                    vkDestroyImageView(logicalDevice, v, m_context->allocator);
                }
            }
            layerViews.Destroy();

            if (m_memory)
            {
                // Determine if memory is device-local (on the GPU)
                const bool isDeviceMemory = m_memoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                const auto size           = m_memoryRequirements.size;
                // Report memory as freed
                MetricsFree(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, size, size, m_memory);

                vkFreeMemory(logicalDevice, m_memory, m_context->allocator);
                m_memory = nullptr;
            }
            if (handle)
            {
                vkDestroyImage(logicalDevice, handle, m_context->allocator);
                handle = nullptr;
            }

            m_name.Destroy();
        }

        std::memset(&m_memoryRequirements, 0, sizeof(VkMemoryRequirements));
    }
}  // namespace C3D
