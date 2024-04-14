
#include "malloc_allocator.h"

#include "core/metrics/metrics.h"

namespace C3D
{
    MallocAllocator::MallocAllocator() : BaseAllocator(ToUnderlying(AllocatorType::Malloc))
    {
        m_id = Metrics.CreateAllocator("MALLOC_ALLOCATOR", AllocatorType::Malloc, 0);
    }

    MallocAllocator::~MallocAllocator() { Metrics.DestroyAllocator(m_id); }

    void* MallocAllocator::AllocateBlock(const MemoryType type, const u64 size, u16 alignment) const
    {
        const auto ptr = std::malloc(size);
        return ptr;
    }

    void MallocAllocator::Free(void* block) const { std::free(block); }

    MallocAllocator* MallocAllocator::GetDefault()
    {
        static auto allocator = new MallocAllocator();
        return allocator;
    }
}  // namespace C3D
