
#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"

#include "core/logger.h"
#include "core/memory.h"
#include "memory/freelist.h"
#include "services/services.h"

namespace C3D
{
	VulkanBuffer::VulkanBuffer(const VulkanContext* context)
		: RenderBuffer("VULKAN_BUFFER"), handle(), m_context(context), m_usage(), m_memory(),
			m_memoryRequirements(), m_memoryIndex(), m_memoryPropertyFlags(), m_isLocked(true)
	{}

	bool VulkanBuffer::Create(const RenderBufferType bufferType, const u64 size, const bool useFreelist)
	{
		if (!RenderBuffer::Create(bufferType, size, useFreelist)) return false;

		const u32 deviceLocalBits = m_context->device.supportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
		switch (bufferType)
		{
			case RenderBufferType::Vertex:
				m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
				m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				break;
			case RenderBufferType::Index:
				m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
				m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				break;
			case RenderBufferType::Uniform:
				m_usage = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
				m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | deviceLocalBits;
				break;
			case RenderBufferType::Staging:
				m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				break;
			case RenderBufferType::Read:
				m_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				break;
			case RenderBufferType::Storage:
				m_logger.Error("Create() - RenderBufferType::Storage is not yet supported.");
				return false;
			case RenderBufferType::Unknown:
				m_logger.Error("Create() - Unsupported buffer type: '{}'.", ToUnderlying(bufferType));
				return false;
		}

		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = m_usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: we assume this is only used in one queue

		VK_CHECK(vkCreateBuffer(m_context->device.logicalDevice, &bufferCreateInfo, m_context->allocator, &handle));

		// Gather memory requirements
		vkGetBufferMemoryRequirements(m_context->device.logicalDevice, handle, &m_memoryRequirements);
		m_memoryIndex = m_context->FindMemoryIndex(m_memoryRequirements.memoryTypeBits, m_memoryPropertyFlags);
		if (m_memoryIndex == -1)
		{
			m_logger.Error("Create() - Unable to create because the required memory type index was not found");
			return false;
		}

		// Allocate memory info
		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = m_memoryRequirements.size;
		allocateInfo.memoryTypeIndex = static_cast<u32>(m_memoryIndex);

		const VkResult result = vkAllocateMemory(m_context->device.logicalDevice, &allocateInfo, m_context->allocator, &m_memory);

		// Determine if memory is on device heap.
		const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		// Report memory as in-use
		Memory.AllocateReport(m_memoryRequirements.size, isDeviceMemory ? MemoryType::GpuLocal : MemoryType::Vulkan);

		if (result != VK_SUCCESS)
		{
			m_logger.Error("Create() - Unable to create because the required memory allocation failed. Error: {}", result);
			return false;
		}

		return true;
	}

	void VulkanBuffer::Destroy()
	{
		RenderBuffer::Destroy();

		if (m_memory)
		{
			vkFreeMemory(m_context->device.logicalDevice, m_memory, m_context->allocator);
			m_memory = nullptr;
		}
		if (handle)
		{
			vkDestroyBuffer(m_context->device.logicalDevice, handle, m_context->allocator);
			handle = nullptr;
		}

		// Report the freeing of the memory.
		const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		Memory.FreeReport(m_memoryRequirements.size, isDeviceMemory ? MemoryType::GpuLocal : MemoryType::Vulkan);
		Memory.Zero(&m_memoryRequirements, sizeof(VkMemoryRequirements));

		totalSize = 0;
		m_usage = {};
		m_isLocked = false;
	}

	bool VulkanBuffer::Bind(const u64 offset)
	{
		VK_CHECK(vkBindBufferMemory(m_context->device.logicalDevice, handle, m_memory, offset));
		return true;
	}

	void* VulkanBuffer::MapMemory(const u64 offset, const u64 size)
	{
		void* data;
		VK_CHECK(vkMapMemory(m_context->device.logicalDevice, m_memory, offset, size, 0, &data));
		return data;
	}

	void VulkanBuffer::UnMapMemory(const u64 offset, u64 size)
	{
		vkUnmapMemory(m_context->device.logicalDevice, m_memory);
	}

	bool VulkanBuffer::Flush(const u64 offset, const u64 size)
	{
		if (!IsHostCoherent())
		{
			VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			range.memory = m_memory;
			range.offset = offset;
			range.size = size;
			VK_CHECK(vkFlushMappedMemoryRanges(m_context->device.logicalDevice, 1, &range));
		}
		return true;
	}

	bool VulkanBuffer::Resize(const u64 newSize)
	{
		RenderBuffer::Resize(newSize);

		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = newSize;
		bufferCreateInfo.usage = m_usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: We assume this is used only in one queue

		VkBuffer newBuffer;
		VK_CHECK(vkCreateBuffer(m_context->device.logicalDevice, &bufferCreateInfo, m_context->allocator, &newBuffer));

		// Gather memory requirements
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_context->device.logicalDevice, newBuffer, &requirements);

		// Allocate memory info 
		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = requirements.size;
		allocateInfo.memoryTypeIndex = static_cast<u32>(m_memoryIndex);

		// Allocate the memory
		VkDeviceMemory newMemory;
		VkResult result = vkAllocateMemory(m_context->device.logicalDevice, &allocateInfo, m_context->allocator, &newMemory);
		if (result != VK_SUCCESS)
		{
			m_logger.Error("Resize() - Unable to resize because the required memory allocation failed. Error: {}", result);
			return false;
		}

		// Bind the new buffer's memory
		VK_CHECK(vkBindBufferMemory(m_context->device.logicalDevice, newBuffer, newMemory, 0));

		// Copy over the data
		CopyRangeInternal(0, newBuffer, 0, totalSize);

		// Make sure anything potentially using these is finished
		vkDeviceWaitIdle(m_context->device.logicalDevice);

		// Destroy the old buffer
		if (m_memory)
		{
			vkFreeMemory(m_context->device.logicalDevice, m_memory, m_context->allocator);
			m_memory = nullptr;
		}
		if (handle)
		{
			vkDestroyBuffer(m_context->device.logicalDevice, handle, m_context->allocator);
			handle = nullptr;
		}

		// Determine if memory is on device heap.
		const bool isDeviceMemory = m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		const auto memoryType = isDeviceMemory ? MemoryType::GpuLocal : MemoryType::Vulkan;

		// Report memory as in-use
		Memory.FreeReport(m_memoryRequirements.size, memoryType);
		m_memoryRequirements = requirements;
		Memory.AllocateReport(m_memoryRequirements.size, memoryType);

		// Set our new properties
		totalSize = newSize;
		m_memory = newMemory;
		handle = newBuffer;

		return true;
	}

	bool VulkanBuffer::Read(const u64 offset, const u64 size, void** outMemory)
	{
		if (!outMemory)
		{
			m_logger.Error("Read() - Requires a valid pointer to outMemory pointer.");
			return false;
		}

		if (IsDeviceLocal() && !IsHostVisible())
		{
			// If the buffer's memory is not host visible but is device local we need a read buffer
			// We use the read buffer to copy the data to it and read from that buffer

			VulkanBuffer read(m_context);
			if (!read.Create(RenderBufferType::Read, size, false))
			{
				m_logger.Error("Read() - Failed to create read buffer.");
				return false;
			}
			read.Bind(0);

			// Perform the copy from device local to the read buffer
			CopyRange(offset, &read, 0, size);

			// Map, copy and unmap
			void* mappedData;
			VK_CHECK(vkMapMemory(m_context->device.logicalDevice, read.m_memory, 0, size, 0, &mappedData));
			Memory.Copy(*outMemory, mappedData, size);
			vkUnmapMemory(m_context->device.logicalDevice, read.m_memory);

			// Cleanup the read buffer
			read.Unbind();
			read.Destroy();
		}
		else
		{
			// We don't need a read buffer can just directly map, copy and unmap
			void* mappedData;
			VK_CHECK(vkMapMemory(m_context->device.logicalDevice, m_memory, 0, size, 0, &mappedData));
			Memory.Copy(*outMemory, mappedData, size);
			vkUnmapMemory(m_context->device.logicalDevice, m_memory);
		}

		return true;
	}

	bool VulkanBuffer::LoadRange(const u64 offset, const u64 size, const void* data)
	{
		if (!data)
		{
			m_logger.Error("LoadRange() - Requires valid data to load.");
			return false;
		}

		if (IsDeviceLocal() && !IsHostVisible())
		{
			// The memory is local but not host visible so we need a staging buffer
			VulkanBuffer staging(m_context);
			if (!staging.Create(RenderBufferType::Staging, size, false))
			{
				m_logger.Error("LoadRange() - Failed to create staging buffer.");
				return false;
			}
			staging.Bind(0);

			// Load the data into the staging buffer
			if (!staging.LoadRange(0, size, data))
			{
				m_logger.Error("LoadRange() - Failed to run Staging::LoadRange().");
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
			void* mappedData;
			VK_CHECK(vkMapMemory(m_context->device.logicalDevice, m_memory, offset, size, 0, &mappedData));
			Memory.Copy(mappedData, data, size);
			vkUnmapMemory(m_context->device.logicalDevice, m_memory);
		}

		return true;
	}

	bool VulkanBuffer::CopyRange(const u64 srcOffset, RenderBuffer* dest, const u64 dstOffset, const u64 size)
	{
		if (!dest || size == 0)
		{
			m_logger.Error("CopyRange() - Requires a valid destination and a nonzero size");
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
		
		m_logger.Error("Draw() - Cannot draw a buffer of type: '{}'.", ToUnderlying(type));
		return false;
	}

	bool VulkanBuffer::IsDeviceLocal() const
	{
		return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	bool VulkanBuffer::IsHostVisible() const
	{
		return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	}

	bool VulkanBuffer::IsHostCoherent() const
	{
		return m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}

	bool VulkanBuffer::CopyRangeInternal(const u64 srcOffset, const VkBuffer dst, const u64 dstOffset, const u64 size) const
	{
		// TODO: Assuming queue and pool usage here. Might want dedicated queue.
		VkQueue queue = m_context->device.graphicsQueue;
		vkQueueWaitIdle(queue);
		// Create a one-time-use command buffer.
		VulkanCommandBuffer tempCommandBuffer;
		tempCommandBuffer.AllocateAndBeginSingleUse(m_context, m_context->device.graphicsCommandPool);

		// Prepare the copy command and add it to the command buffer.
		VkBufferCopy copyRegion;
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		vkCmdCopyBuffer(tempCommandBuffer.handle, handle, dst, 1, &copyRegion);

		// Submit the buffer for execution and wait for it to complete.
		tempCommandBuffer.EndSingleUse(m_context, m_context->device.graphicsCommandPool, queue);

		return true;
	}
}
