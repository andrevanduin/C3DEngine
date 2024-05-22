
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

        Texture(const char* textureName, const TextureType type, const u32 width, const u32 height, const u8 channelCount,
                const TextureFlagBits flags = 0);

        void Set(TextureType _type, const char* _name, u32 _width, u32 _height, u8 _channelCount, TextureFlagBits _flags,
                 void* _internalData = nullptr);

        [[nodiscard]] bool IsWritable() const;
        [[nodiscard]] bool IsWrapped() const;

        u32 id = INVALID_ID;
        String name;

        /** @brief The dimensions of the texture. */
        u32 width = 0, height = 0;
        /** @brief The number of channels in this texture. */
        u8 channelCount = 0;
        /** @brief How many layers this texture has. For non-array textures this is always 1. */
        u16 arraySize = 1;
        /** @brief The amount of mip levels for this texture. Should always be atleast 1 (for the base layer). */
        u8 mipLevels = 1;

        TextureType type;
        TextureFlagBits flags = TextureFlag::None;

        u32 generation     = INVALID_ID;
        void* internalData = nullptr;
    };
}  // namespace C3D
