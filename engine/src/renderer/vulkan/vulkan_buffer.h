
#pragma once
#include "core/defines.h"
#include "vulkan_types.h"
#include "renderer/render_buffer.h"

namespace C3D
{
	class VulkanBuffer final : RenderBuffer
	{
	public:
		VulkanBuffer(const VulkanContext* context, RenderBufferType type, u64 totalSize, u32 usage, u32 memoryPropertyFlags, bool bindOnCreate);

		bool Create(bool useFreelist) override;
		void Destroy() override;

		bool Resize(const VulkanContext* context, u64 newSize, VkQueue queue, VkCommandPool pool);

		void Bind(const VulkanContext* context, u64 offset) const;

		void* LockMemory(const VulkanContext* context, u64 offset, u64 size, u32 flags) const;
		void  UnlockMemory(const VulkanContext* context) const;

		bool Allocate(u64 size, u64* outOffset);
		bool Free(u64 size, u64 offset);

		void LoadData(const VulkanContext* context, u64 offset, u64 size, u32 flags, const void* data) const;

		void CopyTo(const VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, u64 sourceOffset, VkBuffer dest, u64 destOffset, u64 size) const;

		VkBuffer handle;
	private:
		void CleanupFreeList();

		u64 m_totalSize;
		
		VkBufferUsageFlagBits m_usage;

		VkDeviceMemory m_memory;
		VkMemoryRequirements m_memoryRequirements;

		i32 m_memoryIndex;
		u32 m_memoryPropertyFlags;

		bool m_isLocked;
	};
}
