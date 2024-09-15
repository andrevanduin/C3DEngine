
#pragma once
#include "allocators/dynamic_allocator.h"
#include "defines.h"

namespace C3D
{
#ifdef C3D_MEMORY_METRICS_STACKTRACE
#define Allocator(pAlloc) pAlloc->SetStacktrace()
#define Memory C3D::GlobalMemorySystem::GetAllocator().SetStacktraceRef()
#else
#define Allocator(pAlloc) pAlloc
#define Memory C3D::GlobalMemorySystem::GetAllocator()
#endif

#define MemoryUtil C3D::GlobalMemorySystem

    struct MemorySystemConfig
    {
        u64 totalAllocSize    = 0;
        bool excludeFromStats = false;
    };

    class C3D_API GlobalMemorySystem
    {
    public:
        static void Init(const MemorySystemConfig& config);
        static void Destroy();

        static DynamicAllocator& GetAllocator();
    };
}  // namespace C3D
