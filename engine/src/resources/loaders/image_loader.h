
#pragma once
#include "resource_loader.h"

namespace C3D
{
    constexpr auto IMAGE_LOADER_EXTENSION_COUNT = 4;

    struct Image final : Resource
    {
        u8 channelCount;
        u32 width, height;
        u8* pixels;
    };

    struct ImageLoadParams
    {
        /** @brief Indicated if the image should be flipped on the y-axis when loaded. */
        bool flipY = true;
    };

    template <>
    class ResourceLoader<Image> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Load(const char* name, Image& resource) const;
        bool Load(const char* name, Image& resource, const ImageLoadParams& params) const;

        static void Unload(Image& resource);
    };
}  // namespace C3D
