
#pragma once
#include "containers/string.h"
#include "core/defines.h"

namespace C3D
{
    constexpr auto TEXTURE_NAME_MAX_LENGTH = 256;

    enum class TextureUse
    {
        Unknown  = 0x0,
        Diffuse  = 0x1,
        Specular = 0x2,
        Normal   = 0x3,
        CubeMap  = 0x4,
    };

    enum class TextureFilter
    {
        ModeNearest = 0x0,
        ModeLinear  = 0x1,
    };

    enum class TextureRepeat
    {
        Repeat         = 0x1,
        MirroredRepeat = 0x2,
        ClampToEdge    = 0x3,
        ClampToBorder  = 0x4,
    };

    enum TextureFlag
    {
        None = 0x0,
        /* @brief Indicates if the texture has transparency. */
        HasTransparency = 0x1,
        /* @brief Indicates if the texture is writable. */
        IsWritable = 0x2,
        /* @brief Indicates if the texture was created via wrapping vs traditional creation. */
        IsWrapped = 0x4,
        /* @brief Indicates if the texture is being used as a depth texture. */
        IsDepth = 0x8,
    };

    enum class FaceCullMode
    {
        /* @brief No faces are culled. */
        None = 0x0,
        /* @brief Only front faces are culled. */
        Front = 0x1,
        /* @brief Only back faces are culled. */
        Back = 0x2,
        /* @brief Both front and back faces are culled. */
        FrontAndBack = 0x3
    };

    enum class TextureType
    {
        Type2D,
        TypeCube,
    };

    struct ImageResourceData
    {
        u8 channelCount;
        u32 width, height;
        u8* pixels;
    };

    struct ImageResourceParams
    {
        /* @brief Indicated if the image should be flipped on the y-axis when loaded. */
        bool flipY = true;
    };

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

    struct TextureMap
    {
        TextureMap()
            : texture(nullptr),
              use(),
              minifyFilter(),
              magnifyFilter(),
              repeatU(),
              repeatV(),
              repeatW(),
              internalData(nullptr)
        {}

        TextureMap(const TextureUse use, const TextureFilter minifyFilter, const TextureFilter magnifyFilter,
                   const TextureRepeat repeatU, const TextureRepeat repeatV, const TextureRepeat repeatW)
            : texture(nullptr),
              use(use),
              minifyFilter(minifyFilter),
              magnifyFilter(magnifyFilter),
              repeatU(repeatU),
              repeatV(repeatV),
              repeatW(repeatW),
              internalData(nullptr)
        {}

        TextureMap(const TextureUse use, const TextureFilter minifyFilter, const TextureFilter magnifyFilter,
                   const TextureRepeat repeat)
            : texture(nullptr),
              use(use),
              minifyFilter(minifyFilter),
              magnifyFilter(magnifyFilter),
              repeatU(repeat),
              repeatV(repeat),
              repeatW(repeat),
              internalData(nullptr)
        {}

        TextureMap(const TextureUse use, const TextureFilter filter, const TextureRepeat repeat)
            : texture(nullptr),
              use(use),
              minifyFilter(filter),
              magnifyFilter(filter),
              repeatU(repeat),
              repeatV(repeat),
              repeatW(repeat),
              internalData(nullptr)
        {}

        // Pointer to the corresponding texture
        Texture* texture;
        // Use of the texture
        TextureUse use;
        // Texture filtering mode for minification
        TextureFilter minifyFilter;
        // Texture filtering mode for magnification
        TextureFilter magnifyFilter;
        // Texture repeat mode on the U axis
        TextureRepeat repeatU;
        // Texture repeat mode on the V axis
        TextureRepeat repeatV;
        // Texture repeat mode on the W axis
        TextureRepeat repeatW;
        // A pointer to internal, render API-specific data. Typically the internal sampler.
        void* internalData;
    };
}  // namespace C3D
