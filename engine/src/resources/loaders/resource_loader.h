
#pragma once
#include "core/logger.h"
#include "resources/resource_types.h"

namespace C3D
{
    class SystemManager;

    class C3D_API IResourceLoader
    {
    public:
        IResourceLoader(MemoryType memoryType, ResourceType type, const char* customType, const char* path);

        virtual bool Init() { return true; }
        virtual void Shutdown() {}

        u32 id = INVALID_ID;
        ResourceType type;

        String customType;
        String typePath;

    protected:
        MemoryType m_memoryType;
    };

    template <typename T>
    class ResourceLoader : public IResourceLoader
    {
    public:
        explicit ResourceLoader() : IResourceLoader(MemoryType::Unknown, ResourceType::None, nullptr, nullptr) {}

        ResourceLoader(MemoryType memoryType, ResourceType type, const char* customType, const char* path)
            : IResourceLoader(memoryType, type, customType, path)
        {}

        bool Load(const char* name, T& resource) const;
        void Unload(T& resource) const;
    };
}  // namespace C3D
