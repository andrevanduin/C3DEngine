
#pragma once
#include "allocators/dynamic_allocator.h"
#include "core/defines.h"

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

        static void* Zero(void* block, u64 size);

        template <typename T>
        static T* Zero(T* pItem)
        {
            return static_cast<T*>(Zero(pItem, sizeof(T)));
        }

        template <typename T>
        static void* Copy(void* dest, const void* source)
        {
            const auto d = static_cast<T*>(dest);
            const auto s = static_cast<const T*>(source);
            *d           = *s;
            return dest;
        }

        static void* MemCopy(void* dest, const void* source, u64 size);

        static void* SetMemory(void* dest, i32 value, u64 size);

        static DynamicAllocator& GetAllocator();
    };
}  // namespace C3D
