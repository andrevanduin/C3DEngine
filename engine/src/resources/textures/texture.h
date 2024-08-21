
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "resources/textures/texture_types.h"

namespace C3D
{
    typedef u8 TextureFlagBits;

    struct C3D_API Texture
    {
        Texture() = default;

        Texture(const String& n, TextureType t, u32 w, u32 h, u8 channels, u16 layers = 1, TextureFlagBits f = 0)
            : name(n), type(t), width(w), height(h), channelCount(channels), arraySize(layers), flags(f)
        {}

        [[nodiscard]] bool IsWritable() const { return flags & TextureFlag::IsWritable; }
        [[nodiscard]] bool IsWrapped() const { return flags & TextureFlag::IsWrapped; }
        [[nodiscard]] bool HasTransparency() const { return flags & TextureFlag::HasTransparency; }

        u32 handle = INVALID_ID;
        String name;

        /** @brief The dimensions of the texture. */
        u32 width = 0, height = 0;
        /** @brief The number of channels in this texture. */
        u8 channelCount = 0;
        /** @brief How many layers this texture has. For non-array textures this is always 1. */
        u16 arraySize = 1;
        /** @brief The amount of mip levels for this texture. Should always be atleast 1 (for the base layer). */
        u8 mipLevels = 1;

        TextureType type      = TextureTypeNone;
        TextureFlagBits flags = TextureFlag::None;

        u32 generation     = INVALID_ID;
        void* internalData = nullptr;
    };
}  // namespace C3D
