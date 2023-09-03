
#pragma once
#include "resource_loader.h"
#include "resources/texture.h"

namespace C3D
{
    constexpr auto IMAGE_LOADER_EXTENSION_COUNT = 4;

    struct ImageResource final : Resource
    {
        ImageResourceData data;
    };

    template <>
    class ResourceLoader<ImageResource> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, ImageResource& resource) const;
        bool Load(const char* name, ImageResource& resource, const ImageResourceParams& params) const;

        static void Unload(ImageResource& resource);
    };
}  // namespace C3D
