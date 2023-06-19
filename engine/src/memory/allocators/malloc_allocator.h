
#pragma once
#include "base_allocator.h"
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
    struct MallocAllocation
    {
        void* block;
        u64 size;
    };

    class C3D_API MallocAllocator final : public BaseAllocator<MallocAllocator>
    {
    public:
        MallocAllocator();

        MallocAllocator(const MallocAllocator&) = delete;
        MallocAllocator(MallocAllocator&&) = delete;

        MallocAllocator& operator=(const MallocAllocator&) = delete;
        MallocAllocator& operator=(MallocAllocator&&) = delete;

        ~MallocAllocator() override;

        void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) override;
        void Free(MemoryType type, void* block) override;

        static MallocAllocator* GetDefault();

    private:
        LoggerInstance<32> m_logger;
    };
}  // namespace C3D
