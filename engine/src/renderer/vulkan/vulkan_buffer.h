
#pragma once
#include "core/defines.h"
#include "vulkan_types.h"

namespace C3D
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer();

		bool Create(const VulkanContext* context, u64 size, VkBufferUsageFlagBits usage, u32 memoryPropertyFlags, bool bindOnCreate);

		void Destroy(const VulkanContext* context);

		bool Resize(VulkanContext* context, u64 newSize, VkQueue queue, VkCommandPool pool);

		void Bind(const VulkanContext* context, u64 offset) const;

		void* LockMemory(const VulkanContext* context, u64 offset, u64 size, u32 flags) const;
		void  UnlockMemory(const VulkanContext* context) const;

		void LoadData(const VulkanContext* context, u64 offset, u64 size, u32 flags, const void* data) const;

		void CopyTo(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, u64 sourceOffset, VkBuffer dest, u64 destOffset, u64 size) const;
		
	private:
		u64 m_totalSize;

		VkBuffer m_handle;
		VkBufferUsageFlagBits m_usage;

		VkDeviceMemory m_memory;
		i32 m_memoryIndex;
		u32 m_memoryPropertyFlags;

		bool m_isLocked;
	};
}