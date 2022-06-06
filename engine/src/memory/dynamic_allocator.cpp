
#include "dynamic_allocator.h"

#include "core/logger.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	DynamicAllocator::DynamicAllocator(): m_logger("DYNAMIC_ALLOCATOR"), m_initialized(false), m_totalSize(0), m_memory(nullptr) {}

	bool DynamicAllocator::Create(const u64 totalSize, void* memory)
	{
		if (totalSize < 1)
		{
			m_logger.Error("Create() - Size cannot be 0. Creation failed");
			return false;
		}

		m_totalSize = totalSize;
		// The first part of our memory will be used by our freelist
		m_freeList.Create(totalSize, memory);
		// The second part of the memory will store the actual data that this allocator manages
		const u64 freeListMemoryRequirement = FreeList::GetMemoryRequirements(totalSize);
		m_memory = static_cast<u8*>(memory) + freeListMemoryRequirement;

		Memory.Zero(m_memory, totalSize);

		m_initialized = true;
		return true;
	}

	bool DynamicAllocator::Destroy()
	{
		// Destroy our freelist (frees it's memory)
		m_freeList.Destroy();
		// Free the memory we are holding on to
		Memory.Free(m_memory, m_totalSize + FreeList::GetMemoryRequirements(m_totalSize), MemoryType::DynamicAllocator);

		m_totalSize = 0;
		m_memory = nullptr;
		return true;
	}

	void* DynamicAllocator::Allocate(const u64 size)
	{
		if (size == 0)
		{
			Logger::Error("[DYNAMIC_ALLOCATOR] - Allocate() requires a valid size");
			return nullptr;
		}

		u64 offset = 0;
		// Get an offset into our block of memory from our freelist
		if (m_freeList.AllocateBlock(size, &offset)) 
		{
			// To get our requested block we simply add the offset to the start of our memory block
			const auto block = static_cast<u8*>(m_memory) + offset;
			return block;
		}

		// If the freelist cannot find a free block we have no more space
		Logger::Error("[DYNAMIC_ALLOCATOR] - Allocate() could not find a block of memory large enough to allocate from");
		Logger::Error("[DYNAMIC_ALLOCATOR] - Requested size: {}, total space available: {}", size, m_freeList.FreeSpace());
		// TODO: report fragmentation?
		return nullptr;
	}

	bool DynamicAllocator::Free(void* block, const u64 size)
	{
		if (!block || !size)
		{
			Logger::Error("[DYNAMIC_ALLOCATOR] - Free() called with nullptr block or 0 size.");
			return false;
		}

		if (block < m_memory || block > static_cast<u8*>(m_memory) + m_totalSize)
		{
			void* endOfBlock = static_cast<u8*>(m_memory) + m_totalSize;
			Logger::Error("[DYNAMIC_ALLOCATOR] - Free() called with block ({}) outside of allocator range (0x{}) - (0x{})", block, m_memory, endOfBlock);
			return false;
		}

		const u64 offset = static_cast<u8*>(block) - static_cast<u8*>(m_memory);
		if (!m_freeList.FreeBlock(size, offset))
		{
			Logger::Error("[DYNAMIC_ALLOCATOR] - Free() failed");
			return false;
		}
		return true;
	}

	u64 DynamicAllocator::FreeSpace() const
	{
		return m_freeList.FreeSpace();
	}

	u64 DynamicAllocator::GetMemoryRequirements(const u64 totalSize)
	{
		const u64 freeListMemoryRequirement = FreeList::GetMemoryRequirements(totalSize);
		return freeListMemoryRequirement + totalSize;
	}
}
