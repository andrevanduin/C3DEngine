
#pragma once
#include "containers/string.h"
#include "memory/allocators/malloc_allocator.h"
#include "platform/platform.h"

namespace C3D
{
    class C3D_API DynamicLibrary
    {
    public:
        DynamicLibrary(const String& name);

        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary(DynamicLibrary&&)      = delete;

        DynamicLibrary& operator=(const DynamicLibrary&) = delete;
        DynamicLibrary& operator=(DynamicLibrary&&)      = delete;

        ~DynamicLibrary();

        bool Load(const char* name);
        bool Unload();

        template <typename Signature>
        Signature LoadFunction(const char* name)
        {
            return Platform::LoadDynamicLibraryFunction<Signature>(name, m_data);
        }

    protected:
        String m_name;
        String m_fileName;

        u64 m_dataSize = 0;
        void* m_data   = nullptr;
    };
}  // namespace C3D
