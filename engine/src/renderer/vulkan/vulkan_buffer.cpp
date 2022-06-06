
#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"

#include "core/logger.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	VulkanBuffer::VulkanBuffer()
		: handle(nullptr), m_totalSize(0), m_freeListBlock(nullptr), m_freeListMemoryRequirement(0), m_usage(),
		  m_memory(nullptr), m_memoryIndex(0), m_memoryPropertyFlags(0), m_isLocked(false), m_hasFreeList(false)
	{
	}

	bool VulkanBuffer::Create(const VulkanContext* context, const u64 size, const u32 usage, const u32 memoryPropertyFlags, const bool bindOnCreate, bool useFreeList)
	{
		m_hasFreeList = useFreeList;
		m_totalSize = size;
		m_usage = static_cast<VkBufferUsageFlagBits>(usage);
		m_memoryPropertyFlags = memoryPropertyFlags;

		if (m_hasFreeList)
		{
			// Create a new freelist
			m_freeListMemoryRequirement = FreeList::GetMemoryRequirements(size);
			m_freeListBlock = Memory.Allocate(m_freeListMemoryRequirement, MemoryType::RenderSystem);
			m_freeList.Create(size, m_freeListBlock);
		}

		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: we assume this is only used in one queue

		VK_CHECK(vkCreateBuffer(context->device.logicalDevice, &bufferCreateInfo, context->allocator, &handle));

		// Gather memory requirements
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(context->device.logicalDevice, handle, &requirements);
		m_memoryIndex = context->FindMemoryIndex(requirements.memoryTypeBits, m_memoryPropertyFlags);
		if (m_memoryIndex == -1)
		{
			Logger::Error("[VULKAN_BUFFER] - Unable to create because the required memory type index was not found");
			CleanupFreeList();
			return false;
		}

		// Allocate memory info
		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = requirements.size;
		allocateInfo.memoryTypeIndex = static_cast<u32>(m_memoryIndex);

		const VkResult result = vkAllocateMemory(context->device.logicalDevice, &allocateInfo, context->allocator, &m_memory);
		if (result != VK_SUCCESS)
		{
			Logger::Error("[VULKAN_BUFFER] - Unable to create because the required memory allocation failed. Error: {}", result);
			CleanupFreeList();
			return false;
		}

		if (bindOnCreate) Bind(context, 0);

		return true;
	}

	void VulkanBuffer::Destroy(const VulkanContext* context)
	{
		if (m_freeListBlock)
		{
			CleanupFreeList();
		}
		if (m_memory)
		{
			vkFreeMemory(context->device.logicalDevice, m_memory, context->allocator);
		}
		if (handle)
		{
			vkDestroyBuffer(context->device.logicalDevice, handle, context->allocator);
			handle = nullptr;
		}
		m_totalSize = 0;
		m_usage = {};
		m_isLocked = false;
	}

	bool VulkanBuffer::Resize(const VulkanContext* context, const u64 newSize, const VkQueue queue, const VkCommandPool pool)
	{
		if (newSize < m_totalSize)
		{
			Logger::Error("[VULKAN_BUFFER] - Resize() requires the new size to be greater than the old size");
			return false;
		}

		if (m_hasFreeList)
		{
			// Resize our freelist
			const u64 newMemoryRequirement = FreeList::GetMemoryRequirements(newSize);
			void* newBlock = Memory.Allocate(newMemoryRequirement, MemoryType::RenderSystem);
			void* oldBlock;
			if (!m_freeList.Resize(newBlock, newSize, &oldBlock))
			{
				Logger::Error("[VULKAN_BUFFER] - Resize() failed to resize internal freelist");
				Memory.Free(newBlock, newMemoryRequirement, MemoryType::RenderSystem);
				return false;
			}

			// Free our old memory, and assign our new properties
			Memory.Free(oldBlock, m_freeListMemoryRequirement, MemoryType::RenderSystem);
			m_freeListMemoryRequirement = newMemoryRequirement;
			m_freeListBlock = newBlock;
		}

		m_totalSize = newSize;

		// Create a new buffer
		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = newSize;
		bufferCreateInfo.usage = m_usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: we assume this is only used in one queue

		VkBuffer newBuffer;
		VK_CHECK(vkCreateBuffer(context->device.logicalDevice, &bufferCreateInfo, context->allocator, &newBuffer));

		// Gather memory requirements
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(context->device.logicalDevice, newBuffer, &requirements);

		// Allocate memory info 
		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = requirements.size;
		allocateInfo.memoryTypeIndex = static_cast<u32>(m_memoryIndex);

		// Allocate the memory
		VkDeviceMemory newMemory;
		VkResult result = vkAllocateMemory(context->device.logicalDevice, &allocateInfo, context->allocator, &newMemory);
		if (result != VK_SUCCESS)
		{
			Logger::Error("[VULKAN_BUFFER] - Unable to resize because the required memory allocation failed. Error: {}", result);
			return false;
		}

		// Bind the new buffer's memory
		VK_CHECK(vkBindBufferMemory(context->device.logicalDevice, newBuffer, newMemory, 0));

		// Copy over the data
		CopyTo(context, pool, nullptr, queue, 0, newBuffer, 0, m_totalSize);

		// Make sure anything potentially using these is finished
		vkDeviceWaitIdle(context->device.logicalDevice);

		// Destroy the old buffer
		if (m_memory)
		{
			vkFreeMemory(context->device.logicalDevice, m_memory, context->allocator);
			m_memory = nullptr;
		}
		if (handle)
		{
			vkDestroyBuffer(context->device.logicalDevice, handle, context->allocator);
			handle = nullptr;
		}

		// Set our new properties
		m_totalSize = newSize;
		m_memory = newMemory;
		handle = newBuffer;

		return true;
	}

	void VulkanBuffer::Bind(const VulkanContext* context, const u64 offset) const
	{
		VK_CHECK(vkBindBufferMemory(context->device.logicalDevice, handle, m_memory, offset));
	}

	void* VulkanBuffer::LockMemory(const VulkanContext* context, const u64 offset, const u64 size, const u32 flags) const
	{
		void* data;
		VK_CHECK(vkMapMemory(context->device.logicalDevice, m_memory, offset, size, flags, &data));
		return data;
	}

	void VulkanBuffer::UnlockMemory(const VulkanContext* context) const
	{
		vkUnmapMemory(context->device.logicalDevice, m_memory);
	}

	bool VulkanBuffer::Allocate(const u64 size, u64* outOffset)
	{
		if (!size && !outOffset)
		{
			Logger::Error("[VULKAN_BUFFER] - Allocate() called with size = 0 or outOffset = nullptr");
			return false;
		}

		if (!m_hasFreeList)
		{
			Logger::Warn("[VULKAN_BUFFER] - Allocate() called on buffer not using freelists. Offset will not be valid. Call LoadData() instead.");
			*outOffset = 0;
			return true;
		}

		return m_freeList.AllocateBlock(size, outOffset);
	}

	bool VulkanBuffer::Free(const u64 size, const u64 offset)
	{
		if (!size)
		{
			Logger::Error("[VULKAN_BUFFER] - Free() called with size = 0");
			return false;
		}

		if (!m_hasFreeList)
		{
			Logger::Warn("[VULKAN_BUFFER] - Free() called on buffer not using freelists. Nothing was done.");
			return true;
		}

		return m_freeList.FreeBlock(size, offset);
	}

	void VulkanBuffer::LoadData(const VulkanContext* context, const u64 offset, const u64 size, const u32 flags, const void* data) const
	{
		void* dataPtr;
		VK_CHECK(vkMapMemory(context->device.logicalDevice, m_memory, offset, size, flags, &dataPtr));
		Memory.Copy(dataPtr, data, size);
		vkUnmapMemory(context->device.logicalDevice, m_memory);
	}

	void VulkanBuffer::CopyTo(const VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, const u64 sourceOffset, VkBuffer dest, const u64 destOffset, const u64 size) const
	{
		vkQueueWaitIdle(queue);
		// Create a one-time-use command buffer
		VulkanCommandBuffer tempCommandBuffer;
		tempCommandBuffer.AllocateAndBeginSingleUse(context, pool);

		// Prepare the copy command and add it to the command buffer
		VkBufferCopy copyRegion;
		copyRegion.srcOffset = sourceOffset;
		copyRegion.dstOffset = destOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(tempCommandBuffer.handle, handle, dest, 1, &copyRegion);

		// Submit the buffer for execution and wait for it to complete.
		tempCommandBuffer.EndSingleUse(context, pool, queue);
	}

	void VulkanBuffer::CleanupFreeList()
	{
		if (m_hasFreeList)
		{
			m_freeList.Destroy();
			Memory.Free(m_freeListBlock, m_freeListMemoryRequirement, MemoryType::RenderSystem);
			m_freeListMemoryRequirement = 0;
			m_freeListBlock = nullptr;
		}
	}
}
