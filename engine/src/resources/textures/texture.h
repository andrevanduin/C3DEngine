
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

        Texture(const char* textureName, const TextureType type, const u32 width, const u32 height,
                const u8 channelCount, const TextureFlagBits flags = 0);

        void Set(TextureType _type, const char* _name, u32 _width, u32 _height, u8 _channelCount,
                 TextureFlagBits _flags, void* _internalData = nullptr);

        [[nodiscard]] bool IsWritable() const;
        [[nodiscard]] bool IsWrapped() const;

        u32 id    = INVALID_ID;
        u32 width = 0, height = 0;

        TextureType type;

        u8 channelCount       = 0;
        TextureFlagBits flags = TextureFlag::None;

        String name;

        u32 generation     = INVALID_ID;
        void* internalData = nullptr;
    };
}  // namespace C3D
