
#pragma once
#include "resource_loader.h"

namespace C3D
{
    struct BinaryResource final : Resource
    {
        char* data;
        u64 size;
    };

    template <>
    class C3D_API ResourceLoader<BinaryResource> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, BinaryResource& resource) const;
        static void Unload(BinaryResource& resource);
    };
}  // namespace C3D