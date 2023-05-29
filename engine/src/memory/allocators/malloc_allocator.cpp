
#include "malloc_allocator.h"
#include "core/metrics/metrics.h"

namespace C3D
{
	MallocAllocator::MallocAllocator()
		: BaseAllocator(ToUnderlying(AllocatorType::Malloc)), m_logger("MALLOC_ALLOCATOR")
	{
		m_id = Metrics.CreateAllocator("MALLOC_ALLOCATOR", AllocatorType::Malloc, 0);
	}

	MallocAllocator::~MallocAllocator()
	{
		Metrics.DestroyAllocator(m_id);
	}

	void* MallocAllocator::AllocateBlock(const MemoryType type, const u64 size, u16 alignment)
	{
		const auto ptr = std::malloc(size);
#if defined C3D_MEMORY_METRICS_MALLOC && C3D_MEMORY_METRICS_POINTERS
		Metrics.Allocate(m_id, Allocation(type, ptr, size));
#endif
		return ptr;
	}

	void MallocAllocator::Free(const MemoryType type, void* block)
	{
#if defined C3D_MEMORY_METRICS_MALLOC && C3D_MEMORY_METRICS_POINTERS
		Metrics.Free(m_id, DeAllocation(type, block));
#endif
		std::free(block);
	}
}
