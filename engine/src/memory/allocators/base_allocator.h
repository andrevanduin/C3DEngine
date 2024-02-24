
#pragma once
#include "core/defines.h"
#include "core/metrics/metrics.h"
#include "core/metrics/types.h"

namespace C3D
{
    template <class Derived>
    class C3D_API BaseAllocator
    {
    public:
        explicit BaseAllocator(const u8 type) : m_type(type) {}

        BaseAllocator(const BaseAllocator&) = delete;
        BaseAllocator(BaseAllocator&&)      = delete;

        BaseAllocator& operator=(const BaseAllocator&) = delete;
        BaseAllocator& operator=(BaseAllocator&&)      = delete;

        virtual ~BaseAllocator() = default;

        virtual void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) const = 0;
        virtual void Free(void* block) const                                            = 0;

        virtual bool GetSizeAlignment(void* block, u64* outSize, u16* outAlignment) { return true; }

        virtual bool GetAlignment(void* block, u16* outAlignment) { return true; }
        virtual bool GetAlignment(const void* block, u16* outAlignment) { return true; }

#ifdef C3D_MEMORY_METRICS_STACKTRACE
        BaseAllocator& SetStacktraceRef()
        {
            Metrics.SetStacktrace();
            return *this;
        }

        BaseAllocator* SetStacktrace()
        {
            Metrics.SetStacktrace();
            return this;
        }
#endif

        template <typename T>
        C3D_INLINE T* Allocate(const MemoryType type, const u64 count = 1) const
        {
            return static_cast<T*>(AllocateBlock(type, sizeof(T) * count, alignof(T)));
        }

        template <class T, class... Args>
        C3D_INLINE T* New(const MemoryType type, Args&&... args) const
        {
            return new (AllocateBlock(type, sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
        }

        template <class T>
        void Delete(T* instance) const
        {
            // Call our destructor
            instance->~T();
            // Free our memory
            Free(instance);
        }

        [[nodiscard]] void* GetMemory() const { return m_memoryBlock; }

        [[nodiscard]] u8 GetId() const { return m_id; }

        static Derived* GetDefault() { return Derived::GetDefault(); }

    protected:
        // The id for this allocator
        u8 m_id = INVALID_ID_U8;
        u8 m_type;

        char* m_memoryBlock = nullptr;
    };
}  // namespace C3D
