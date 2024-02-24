
#pragma once
#include "core/logger.h"
#include "resources/resource_types.h"

namespace C3D
{
    class SystemManager;

    class C3D_API IResourceLoader
    {
    public:
        IResourceLoader(const SystemManager* pSystemsManager, MemoryType memoryType, ResourceType type, const char* customType,
                        const char* path);

        IResourceLoader(const IResourceLoader& other) = delete;
        IResourceLoader(IResourceLoader&& other)      = delete;

        IResourceLoader& operator=(const IResourceLoader& other) = default;
        IResourceLoader& operator=(IResourceLoader&& other)      = delete;

        virtual ~IResourceLoader() = default;

        u32 id = INVALID_ID;
        ResourceType type;

        String customType;
        String typePath;

    protected:
        MemoryType m_memoryType;

        const SystemManager* m_pSystemsManager = nullptr;
    };

    template <typename T>
    class ResourceLoader : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager)
            : IResourceLoader(pSystemsManager, MemoryType::Unknown, ResourceType::None, nullptr, nullptr)
        {}

        ResourceLoader(const SystemManager* pSystemsManager, MemoryType memoryType, ResourceType type, const char* customType,
                       const char* path)
            : IResourceLoader(pSystemsManager, memoryType, type, customType, path)
        {}

        bool Load(const char* name, T& resource) const;
        void Unload(T& resource) const;
    };
}  // namespace C3D
