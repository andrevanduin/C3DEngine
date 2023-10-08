
#pragma once
#include "core/logger.h"
#include "resources/resource_types.h"

namespace C3D
{
    class SystemManager;

    class C3D_API IResourceLoader
    {
    public:
        IResourceLoader(const SystemManager* pSystemsManager, const char* name, MemoryType memoryType,
                        ResourceType type, const char* customType, const char* path);

        IResourceLoader(const IResourceLoader& other) = delete;
        IResourceLoader(IResourceLoader&& other)      = delete;

        IResourceLoader& operator=(const IResourceLoader& other) = default;
        IResourceLoader& operator=(IResourceLoader&& other)      = delete;

        virtual ~IResourceLoader() = default;

        u32 id;
        ResourceType type;

        String customType;
        String typePath;

    protected:
        LoggerInstance<64> m_logger;
        MemoryType m_memoryType;

        const SystemManager* m_pSystemsManager;
    };

    template <typename T>
    class ResourceLoader : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager)
            : IResourceLoader(pSystemsManager, "NONE", MemoryType::Unknown, ResourceType::None, nullptr, nullptr)
        {}

        ResourceLoader(const SystemManager* pSystemsManager, const char* name, MemoryType memoryType, ResourceType type,
                       const char* customType, const char* path)
            : IResourceLoader(pSystemsManager, name, memoryType, type, customType, path)
        {}

        bool Load(const char* name, T& resource) const;
        static void Unload(T& resource);
    };
}  // namespace C3D
