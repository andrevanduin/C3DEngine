
#pragma once
#include "base_allocator.h"
#include "core/defines.h"
#include "core/logger.h"
#include "core/metrics/metrics.h"

namespace C3D
{
    template <u64 Size>
    class C3D_API StackAllocator final : public BaseAllocator<StackAllocator<Size>>
    {
    public:
        StackAllocator()
            : BaseAllocator<StackAllocator>(ToUnderlying(AllocatorType::Stack)),
              m_logger("STACK_ALLOCATOR"),
              m_allocated(0)
        {}

        void Create(const char* name)
        {
            this->m_id = Metrics.CreateAllocator(name, AllocatorType::Stack, Size);
            std::memset(m_memory.Data(), 0, Size);
        }

        void Destroy() { FreeAll(); }

        void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) override
        {
            if (m_allocated + size > Size)
            {
                throw std::bad_alloc();
            }

            const auto dataPtr = &m_memory[m_allocated];
            m_allocated += size;

#ifdef C3D_MEMORY_METRICS
#ifdef C3D_MEMORY_METRICS_POINTERS
            Metrics.Allocate(this->m_id, Allocation(type, dataPtr, size));
#else
            Metrics.Allocate(this->m_id, Allocation(type, size));
#endif
#endif

            return dataPtr;
        }

        void Free(MemoryType type, void* block) override {}

        void FreeAll()
        {
            std::memset(m_memory.Data(), 0, Size);
            m_allocated = 0;

            Metrics.FreeAll(this->m_id);
        }

        [[nodiscard]] static u64 GetTotalSize() { return Size; }
        [[nodiscard]] u64 GetAllocated() const { return m_allocated; }

        static StackAllocator* GetDefault()
        {
            static auto allocator = new StackAllocator<KibiBytes(8)>();
            return allocator;
        }

    private:
        LoggerInstance<32> m_logger;

        Array<char, Size> m_memory;
        u64 m_allocated;
    };
}  // namespace C3D
