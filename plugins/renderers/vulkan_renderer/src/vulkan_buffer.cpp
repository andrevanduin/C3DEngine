
#include "vulkan_buffer.h"

#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <platform/platform.h>

#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_formatters.h"
#include "vulkan_utils.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "VULKAN_BUFFER";

    VulkanBuffer::VulkanBuffer(const VulkanContext* context, const String& name) : RenderBuffer(name), m_context(context) {}

    bool VulkanBuffer::Create(const RenderBufferType bufferType, const u64 size, const bool useFreelist)
    {
        if (!RenderBuffer::Create(bufferType, size, useFreelist)) return false;

        u32 deviceLocalBits = 0;
        if (m_context->device.HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_DEVICE_LOCAL_HOST_VISIBILE_MEMORY_BIT))
        {
            deviceLocalBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        switch (bufferType)
        {
            case RenderBufferType::Vertex:
                m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case RenderBufferType::Index:
                m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case RenderBufferType::Uniform:
                m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
                m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | deviceLocalBits;
                break;
            case RenderBufferType::Staging:
                m_usage               = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case RenderBufferType::Read:
                m_usage               = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case RenderBufferType::Storage:
                ERROR_LOG("RenderBufferType::Storage is not yet supported.");
                return false;
            case RenderBufferType::Unknown:
                ERROR_LOG("Unsupported buffer type: '{}'.", ToUnderlying(bufferType));
                return false;
        }

        VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size               = size;
        bufferCreateInfo.usage              = m_usage;
        // NOTE: we assume this is only used in one queue
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto logicalDevice = m_context->device.GetLogical();

        VK_CHECK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, m_context->allocator, &handle));

        // Gather memory requirements
        vkGetBufferMemoryRequirements(logicalDevice, handle, &m_memoryRequirements);
        m_memoryIndex = m_context->device.FindMemoryIndex(m_memoryRequirements.memoryTypeBits, m_memoryPropertyFlags);
        if (m_memoryIndex == -1)
        {
            ERROR_LOG("Unable to create because the required memory type index was not found.");
            return false;
        }

        // Allocate memory info
        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize       = m_memoryRequirements.size;
        allocateInfo.memoryTypeIndex      = static_cast<u32>(m_memoryIndex);

        const VkResult result = vkAllocateMemory(logicalDevice, &allocateInfo, m_context->allocator, &m_memory);

        // Determine if memory is on device heap.
        const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto memSize        = m_memoryRequirements.size;
        // Report memory as in-use
        MetricsAllocate(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, memSize, memSize, m_memory);

        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_DEVICE_MEMORY, m_memory, m_name);

        if (result != VK_SUCCESS)
        {
            ERROR_LOG("Unable to create because the required memory allocation failed. Error: {}.", result);
            return false;
        }

        return true;
    }

    void VulkanBuffer::Destroy()
    {
        RenderBuffer::Destroy();

        // Report the freeing of the memory.
        const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        MetricsFree(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, m_memoryRequirements.size,
                    m_memoryRequirements.size, m_memory);

        m_context->device.WaitIdle();

        auto logicalDevice = m_context->device.GetLogical();
        if (m_memory)
        {
            vkFreeMemory(logicalDevice, m_memory, m_context->allocator);
            m_memory = nullptr;
        }
        if (handle)
        {
            vkDestroyBuffer(logicalDevice, handle, m_context->allocator);
            handle = nullptr;
        }

        std::memset(&m_memoryRequirements, 0, sizeof(VkMemoryRequirements));

        totalSize  = 0;
        m_usage    = {};
        m_isLocked = false;
    }

    bool VulkanBuffer::Bind(const u64 offset)
    {
        VK_CHECK(vkBindBufferMemory(m_context->device.GetLogical(), handle, m_memory, offset));
        return true;
    }

    void* VulkanBuffer::MapMemory(const u64 offset, const u64 size)
    {
        void* data;
        VK_CHECK(vkMapMemory(m_context->device.GetLogical(), m_memory, offset, size, 0, &data));
        return data;
    }

    void VulkanBuffer::UnMapMemory(const u64 offset, u64 size) { vkUnmapMemory(m_context->device.GetLogical(), m_memory); }

    bool VulkanBuffer::Flush(const u64 offset, const u64 size)
    {
        if (!IsHostCoherent())
        {
            VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
            range.memory              = m_memory;
            range.offset              = offset;
            range.size                = size;
            VK_CHECK(vkFlushMappedMemoryRanges(m_context->device.GetLogical(), 1, &range));
        }
        return true;
    }

    bool VulkanBuffer::Resize(const u64 newSize)
    {
        RenderBuffer::Resize(newSize);

        VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size               = newSize;
        bufferCreateInfo.usage              = m_usage;
        // NOTE: We assume this is used only in one queue
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto logicalDevice = m_context->device.GetLogical();

        VkBuffer newBuffer;
        VK_CHECK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, m_context->allocator, &newBuffer));

        // Gather memory requirements
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(logicalDevice, newBuffer, &requirements);

        // Allocate memory info
        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize       = requirements.size;
        allocateInfo.memoryTypeIndex      = static_cast<u32>(m_memoryIndex);

        // Allocate the memory
        VkDeviceMemory newMemory;
        const VkResult result = vkAllocateMemory(logicalDevice, &allocateInfo, m_context->allocator, &newMemory);
        if (result != VK_SUCCESS)
        {
            ERROR_LOG("Unable to resize because the required memory allocation failed. Error: {}.", result);
            return false;
        }

        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_DEVICE_MEMORY, newMemory, m_name);

        // Bind the new buffer's memory
        VK_CHECK(vkBindBufferMemory(logicalDevice, newBuffer, newMemory, 0));

        // Copy over the data
        CopyRangeInternal(0, newBuffer, 0, totalSize);

        // Make sure anything potentially using these is finished
        m_context->device.WaitIdle();

        // Determine if memory is on device heap.
        const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto size           = m_memoryRequirements.size;

        // Report the free of our old allocation
        MetricsFree(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, size, size, m_memory);

        // Destroy the old buffer
        if (m_memory)
        {
            vkFreeMemory(logicalDevice, m_memory, m_context->allocator);
            m_memory = nullptr;
        }
        if (handle)
        {
            vkDestroyBuffer(logicalDevice, handle, m_context->allocator);
            handle = nullptr;
        }

        // Set the new memory requirements
        m_memoryRequirements = requirements;

        // Set our new properties
        totalSize = newSize;
        m_memory  = newMemory;
        handle    = newBuffer;

        // Report memory as in-use
        MetricsAllocate(isDeviceMemory ? GPU_ALLOCATOR_ID : Memory.GetId(), MemoryType::Vulkan, size, size, m_memory);

        return true;
    }

    bool VulkanBuffer::Read(const u64 offset, const u64 size, void** outMemory)
    {
        if (!outMemory)
        {
            ERROR_LOG("Requires a valid pointer to outMemory pointer.");
            return false;
        }

        if (IsDeviceLocal() && !IsHostVisible())
        {
            // If the buffer's memory is not host visible but is device local we need a read buffer
            // We use the read buffer to copy the data to it and read from that buffer

            VulkanBuffer read(m_context, "READ_BUFFER");
            if (!read.Create(RenderBufferType::Read, size, false))
            {
                ERROR_LOG("Failed to create read buffer.");
                return false;
            }
            read.Bind(0);

            // Perform the copy from device local to the read buffer
            CopyRange(offset, &read, 0, size);

            auto logicalDevice = m_context->device.GetLogical();

            // Map, copy and unmap
            void* mappedData;
            VK_CHECK(vkMapMemory(logicalDevice, read.m_memory, 0, size, 0, &mappedData));
            std::memcpy(*outMemory, mappedData, size);

            vkUnmapMemory(logicalDevice, read.m_memory);

            // Cleanup the read buffer
            read.Unbind();
            read.Destroy();
        }
        else
        {
            // We don't need a read buffer can just directly map, copy and unmap
            auto logicalDevice = m_context->device.GetLogical();

            void* mappedData;
            VK_CHECK(vkMapMemory(logicalDevice, m_memory, 0, size, 0, &mappedData));
            std::memcpy(*outMemory, mappedData, size);
            vkUnmapMemory(logicalDevice, m_memory);
        }

        return true;
    }

    bool VulkanBuffer::LoadRange(const u64 offset, const u64 size, const void* data)
    {
        if (!data)
        {
            ERROR_LOG("Requires valid data to load.");
            return false;
        }

        if (IsDeviceLocal() && !IsHostVisible())
        {
            // The memory is local but not host visible so we need a staging buffer
            VulkanBuffer staging(m_context, "LOAD_RANGE_STAGING_BUFFER");
            if (!staging.Create(RenderBufferType::Staging, size, false))
            {
                ERROR_LOG("Failed to create staging buffer.");
                return false;
            }
            staging.Bind(0);

            // Load the data into the staging buffer
            if (!staging.LoadRange(0, size, data))
            {
                ERROR_LOG("Failed to run Staging::LoadRange().");
                return false;
            }

            // Perform the copy from the staging to the device local buffer
            staging.CopyRangeInternal(0, handle, offset, size);

            // Cleanup the staging buffer
            staging.Unbind();
            staging.Destroy();
        }
        else
        {
            // If we don't need a staging buffer we just map, copy and unmap directly
            auto logicalDevice = m_context->device.GetLogical();

            void* mappedData;
            VK_CHECK(vkMapMemory(logicalDevice, m_memory, offset, size, 0, &mappedData));
            std::memcpy(mappedData, data, size);
            vkUnmapMemory(logicalDevice, m_memory);
        }

        return true;
    }

    bool VulkanBuffer::CopyRange(const u64 srcOffset, RenderBuffer* dest, const u64 dstOffset, const u64 size)
    {
        if (!dest || size == 0)
        {
            ERROR_LOG("Requires a valid destination and a nonzero size.");
            return false;
        }
        return CopyRangeInternal(srcOffset, dynamic_cast<VulkanBuffer*>(dest)->handle, dstOffset, size);
    }

    bool VulkanBuffer::Draw(const u64 offset, const u32 elementCount, const bool bindOnly)
    {
        const VulkanCommandBuffer* commandBuffer = &m_context->graphicsCommandBuffers[m_context->imageIndex];

        if (type == RenderBufferType::Vertex)
        {
            const VkDeviceSize offsets[1] = { offset };
            vkCmdBindVertexBuffers(commandBuffer->handle, 0, 1, &handle, offsets);
            if (!bindOnly)
            {
                vkCmdDraw(commandBuffer->handle, elementCount, 1, 0, 0);
            }
            return true;
        }

        if (type == RenderBufferType::Index)
        {
            vkCmdBindIndexBuffer(commandBuffer->handle, handle, offset, VK_INDEX_TYPE_UINT32);
            if (!bindOnly)
            {
                vkCmdDrawIndexed(commandBuffer->handle, elementCount, 1, 0, 0, 0);
            }
            return true;
        }

        ERROR_LOG("Cannot draw a buffer of type: '{}'.", ToUnderlying(type));
        return false;
    }

    bool VulkanBuffer::IsDeviceLocal() const { return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; }

    bool VulkanBuffer::IsHostVisible() const { return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; }

    bool VulkanBuffer::IsHostCoherent() const { return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; }

    bool VulkanBuffer::CopyRangeInternal(const u64 srcOffset, const VkBuffer dst, const u64 dstOffset, const u64 size) const
    {
        // TODO: Assuming queue and pool usage here. Might want dedicated queue.
        auto graphicsCommandPool = m_context->device.GetGraphicsCommandPool();
        auto queue               = m_context->device.GetGraphicsQueue();

        vkQueueWaitIdle(queue);
        // Create a one-time-use command buffer.
        VulkanCommandBuffer tempCommandBuffer;
        tempCommandBuffer.AllocateAndBeginSingleUse(m_context, graphicsCommandPool);

        // Prepare the copy command and add it to the command buffer.
        VkBufferCopy copyRegion;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size      = size;
        vkCmdCopyBuffer(tempCommandBuffer.handle, handle, dst, 1, &copyRegion);

        // Submit the buffer for execution and wait for it to complete.
        tempCommandBuffer.EndSingleUse(m_context, graphicsCommandPool, queue);

        return true;
    }
}  // namespace C3D
