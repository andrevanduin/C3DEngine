
#pragma once
#include "resource_manager.h"

namespace C3D
{
    struct BinaryResource final : IResource
    {
        char* data;
        u64 size;
    };

    template <>
    class C3D_API ResourceManager<BinaryResource> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, BinaryResource& resource) const;
        void Cleanup(BinaryResource& resource) const;
    };
}  // namespace C3D