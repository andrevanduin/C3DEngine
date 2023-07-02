
#include "texture.h"

#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    Texture::Texture()
        : id(0), width(0), height(0), type(), channelCount(0), flags(0), generation(INVALID_ID), internalData(nullptr)
    {}

    Texture::Texture(const char* textureName, const TextureType type, const u32 width, const u32 height,
                     const u8 channelCount, const TextureFlagBits flags /* = 0*/)
        : id(INVALID_ID),
          width(width),
          height(height),
          type(type),
          channelCount(channelCount),
          flags(flags),
          generation(INVALID_ID),
          internalData(nullptr)
    {
        name = textureName;
    }

    void Texture::Set(const u32 _id, TextureType _type, const char* _name, u32 _width, u32 _height, u8 _channelCount,
                      TextureFlagBits _flags, void* _internalData /*  = nullptr*/)
    {
        id = _id;
        type = _type;
        name = _name;
        width = _width;
        height = _height;
        channelCount = _channelCount;
        flags = _flags;
        internalData = _internalData;
    }

    bool Texture::IsWritable() const { return flags & TextureFlag::IsWritable; }
    bool Texture::IsWrapped() const { return flags & TextureFlag::IsWrapped; }
}  // namespace C3D