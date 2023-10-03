
#include "texture.h"

#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    Texture::Texture(const char* textureName, const TextureType type, const u32 width, const u32 height,
                     const u8 channelCount, const TextureFlagBits flags /* = 0*/)
        : width(width), height(height), type(type), channelCount(channelCount), flags(flags), name(textureName)
    {}

    void Texture::Set(TextureType type, const char* name, u32 width, u32 height, u8 channelCount, TextureFlagBits flags,
                      void* internalData /*  = nullptr*/)
    {
        this->type         = type;
        this->name         = name;
        this->width        = width;
        this->height       = height;
        this->channelCount = channelCount;
        this->flags        = flags;
        this->internalData = internalData;
    }

    bool Texture::IsWritable() const { return flags & TextureFlag::IsWritable; }
    bool Texture::IsWrapped() const { return flags & TextureFlag::IsWrapped; }
}  // namespace C3D