
#include "linear_allocator.h"

#include "core/logger.h"
#include "core/metrics/metrics.h"
#include "memory/global_memory_system.h"
#include "platform/platform.h"

namespace C3D
{
	LinearAllocator::LinearAllocator()
		: BaseAllocator(ToUnderlying(AllocatorType::Linear)), m_logger("LINEAR_ALLOCATOR"),
	      m_totalSize(0), m_allocated(0), m_ownsMemory(false)
	{}

	void LinearAllocator::Create(const char* name, const u64 totalSize, void* memory)
	{
		m_totalSize = totalSize;
		m_allocated = 0;
		m_ownsMemory = memory == nullptr;

		// The memory already exists is thus owned by someone else
		if (memory) m_memoryBlock = static_cast<char*>(memory);
		else
		{
			// We need to allocate the memory ourselves
			m_memoryBlock = Memory.Allocate<char>(MemoryType::LinearAllocator, totalSize);
		}

		// Create an metrics object to track the allocations this linear allocator is doing
		m_id = Metrics.CreateAllocator(name, AllocatorType::Linear, totalSize);
	}

	void LinearAllocator::Destroy()
	{
		// First we free all our memory
		FreeAll();
		// Then if we actually own the memory block we free the block
		if (m_ownsMemory && m_memoryBlock)
		{
			// We own the memory so let's free it
			Memory.Free(MemoryType::LinearAllocator, m_memoryBlock);
		}
		// Destroy the metrics object associated with this allocator
		Metrics.DestroyAllocator(m_id, false);

		m_memoryBlock = nullptr;
		m_totalSize = 0;
		m_ownsMemory = false;
	}

	void* LinearAllocator::AllocateBlock(const MemoryType type, const u64 size, u16 alignment)
	{
		if (m_memoryBlock)
		{
			if (m_allocated + size > m_totalSize)
			{
				throw std::bad_alloc();
			}

			char* block = m_memoryBlock + m_allocated;
			m_allocated += size;

			MetricsAllocate(m_id, type, size, size, block);

			std::memset(block, 0, size);
			return block;
		}

		m_logger.Error("Not initialized");
		return nullptr;
	}

	void LinearAllocator::Free(MemoryType type, void* block) {}

	void LinearAllocator::FreeAll()
	{
		if (m_memoryBlock)
		{
			m_allocated = 0;
			std::memset(m_memoryBlock, 0, m_totalSize);

			// Ensure that the metrics keep track of the fact that we just freed all memory for this allocator
			Metrics.FreeAll(m_id);
		}
	}

	LinearAllocator* LinearAllocator::GetDefault()
	{
		static auto allocator = new LinearAllocator();
		return allocator;
	}
}
