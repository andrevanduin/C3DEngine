
#include "malloc_allocator.h"

namespace C3D
{
	MallocAllocator::MallocAllocator()
		: BaseAllocator(ToUnderlying(AllocatorType::Malloc)), m_logger("MALLOC_ALLOCATOR"), m_allocations()
	{
		m_id = Metrics.CreateAllocator("MALLOC_ALLOCATOR", AllocatorType::Malloc, 0);
	}

	MallocAllocator::~MallocAllocator()
	{
		Metrics.DestroyAllocator(m_id);
	}

	void* MallocAllocator::AllocateBlock(const MemoryType type, const u64 size, u16 alignment)
	{
#ifdef C3D_DEBUG_MEMORY_USAGE
	#ifdef C3D_DEBUG_MALLOC_ALLOCATOR
		Metrics.Allocate(m_id, type, size);
	#endif
#endif
		return std::malloc(size);
	}

	void MallocAllocator::Free(const MemoryType type, void* block)
	{
#ifdef C3D_DEBUG_MEMORY_USAGE
	#ifdef C3D_DEBUG_MALLOC_ALLOCATOR
		u64 size = 0;
		for (const auto& [b, s] : m_allocations)
		{
			if (block == b)
			{
				size = s;
				break;
			}
		}
		Metrics.Free(m_id, type, size);
	#endif
#endif
		std::free(block);
	}

	u64 MallocAllocator::GetTotalAllocated() const
	{
		u64 total = 0;
		for (const auto& [block, size] : m_allocations)
		{
			if (block)
			{
				total += size;
			}
		}
		return total;
	}
}
