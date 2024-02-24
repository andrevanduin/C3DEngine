
#include "vulkan_image.h"

#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <platform/platform.h>
#include <systems/system_manager.h>

#include "vulkan_utils.h"

namespace C3D
{
    VulkanImage::~VulkanImage() { Destroy(); }

    VkImageType GetVkImageType(const TextureType type)
    {
        /*
        switch (type)
        {
                case TextureType::Type2D:
                        return VK_IMAGE_TYPE_2D;
                case TextureType::TypeCube:
                        return VK_IMAGE_TYPE_2D;
        }*/ // TODO: This currently is not used
        return VK_IMAGE_TYPE_2D;
    }

    VkImageViewType GetVkImageViewType(const TextureType type)
    {
        switch (type)
        {
            case TextureType::Type2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType::TypeCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
        }
        return VK_IMAGE_VIEW_TYPE_2D;
    }

    void VulkanImage::Create(const VulkanContext* context, const String& name, const TextureType type, const u32 _width, const u32 _height,
                             const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage,
                             const VkMemoryPropertyFlags memoryFlags, const bool createView, const VkImageAspectFlags viewAspectFlags)
    {
        m_context     = context;
        m_name        = name;
        width         = _width;
        height        = _height;
        m_memoryFlags = memoryFlags;

        VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCreateInfo.imageType         = GetVkImageType(type);
        imageCreateInfo.extent.width      = width;
        imageCreateInfo.extent.height     = height;
        // TODO: Support different depth.
        imageCreateInfo.extent.depth = 1;
        // TODO: Support MipMapping.
        imageCreateInfo.mipLevels     = 4;
        imageCreateInfo.arrayLayers   = type == TextureType::TypeCube ? 6 : 1;
        imageCreateInfo.format        = format;
        imageCreateInfo.tiling        = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage         = usage;
        // TODO: Configurable sample count.
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        // TODO: Configurable sharing mode.
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (type == TextureType::TypeCube)
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
            Logger::Error("[VULKAN_IMAGE ({})] - Required memory type not found. Image not valid.", m_name);
            return;
        }

        VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize       = m_memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex      = memoryType;
        VK_CHECK(vkAllocateMemory(logicalDevice, &memoryAllocateInfo, context->allocator, &m_memory));

        VK_SET_DEBUG_OBJECT_NAME(context, VK_OBJECT_TYPE_DEVICE_MEMORY, m_memory, m_name);

        // TODO: Configurable memory offset.
        VK_CHECK(vkBindImageMemory(logicalDevice, handle, m_memory, 0));

        // Determine if memory is device local (on the GPU)
        const bool isDeviceMemory = m_memoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto size           = m_memoryRequirements.size;
        // Report memory as in-use
        MetricsAllocate(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, size, size, m_memory);

        if (createView)
        {
            view = nullptr;
            CreateView(type, format, viewAspectFlags);
        }
    }

    void VulkanImage::CreateView(TextureType type, const VkFormat format, const VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewCreateInfo       = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCreateInfo.image                       = handle;
        viewCreateInfo.viewType                    = GetVkImageViewType(type);
        viewCreateInfo.format                      = format;
        viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

        // TODO: Make Configurable.
        viewCreateInfo.subresourceRange.baseMipLevel   = 0;
        viewCreateInfo.subresourceRange.levelCount     = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount     = type == TextureType::TypeCube ? 6 : 1;

        VK_CHECK(vkCreateImageView(m_context->device.GetLogical(), &viewCreateInfo, m_context->allocator, &view));

        const auto viewName = m_name + "_view";
        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_IMAGE_VIEW, view, viewName);
    }

    void VulkanImage::TransitionLayout(const VulkanCommandBuffer* commandBuffer, const TextureType type, VkFormat format,
                                       const VkImageLayout oldLayout, const VkImageLayout newLayout) const
    {
        auto graphicsQueueIndex = m_context->device.GetGraphicsQueueIndex();

        VkImageMemoryBarrier barrier            = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout                       = oldLayout;
        barrier.newLayout                       = newLayout;
        barrier.srcQueueFamilyIndex             = graphicsQueueIndex;
        barrier.dstQueueFamilyIndex             = graphicsQueueIndex;
        barrier.image                           = handle;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = type == TextureType::TypeCube ? 6 : 1;

        VkPipelineStageFlags sourceStage, destStage;

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
            Logger::Fatal("[VULKAN_IMAGE ({})] - Unsupported layout transition", m_name);
            return;
        }

        vkCmdPipelineBarrier(commandBuffer->handle, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VulkanImage::CopyFromBuffer(const TextureType type, VkBuffer buffer, u64 offset, const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = offset;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = type == TextureType::TypeCube ? 6 : 1;

        region.imageExtent.width  = width;
        region.imageExtent.height = height;
        region.imageExtent.depth  = 1;

        vkCmdCopyBufferToImage(commandBuffer->handle, buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void VulkanImage::CopyToBuffer(const TextureType type, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = type == TextureType::TypeCube ? 6 : 1;

        region.imageExtent.width  = width;
        region.imageExtent.height = height;
        region.imageExtent.depth  = 1;

        vkCmdCopyImageToBuffer(commandBuffer->handle, handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
    }

    void VulkanImage::CopyPixelToBuffer(const TextureType type, VkBuffer buffer, const u32 x, const u32 y,
                                        const VulkanCommandBuffer* commandBuffer) const
    {
        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = type == TextureType::TypeCube ? 6 : 1;

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
        }

        std::memset(&m_memoryRequirements, 0, sizeof(VkMemoryRequirements));
    }
}  // namespace C3D
