
#pragma once
#include "resource_manager.h"

namespace C3D
{
    constexpr auto IMAGE_LOADER_EXTENSION_COUNT = 4;

    struct Image final : public IResource
    {
        Image() : IResource(ResourceType::Image) {}

        u8 channelCount = 0;
        u32 width       = 0;
        u32 height      = 0;
        u8* pixels      = nullptr;
        /** @brief The amount of mip levels to be generated for this image resource.
         * This value should always be atleast 1 since we will always have atleast the base image.
         */
        u8 mipLevels = 1;
    };

    struct ImageLoadParams
    {
        /** @brief Indicated if the image should be flipped on the y-axis when loaded. */
        bool flipY = true;
    };

    template <>
    class ResourceManager<Image> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Read(const String& name, Image& resource) const;
        bool Read(const String& name, Image& resource, const ImageLoadParams& params) const;
        void Cleanup(Image& resource) const;
    };
}  // namespace C3D
