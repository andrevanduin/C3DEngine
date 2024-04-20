
#pragma once

namespace C3D
{
    constexpr auto TEXTURE_NAME_MAX_LENGTH = 256;

    enum class TextureFilter
    {
        ModeNearest = 0x0,
        ModeLinear  = 0x1,
    };

    static inline const char* ToString(TextureFilter filter)
    {
        switch (filter)
        {
            case TextureFilter::ModeNearest:
                return "nearest";
            case TextureFilter::ModeLinear:
                return "linear";
            default:
                C3D_ASSERT_MSG(false, "Invalid TextureFilter");
                return "ERROR";
        }
    }

    enum class TextureRepeat
    {
        Repeat         = 0x1,
        MirroredRepeat = 0x2,
        ClampToEdge    = 0x3,
        ClampToBorder  = 0x4,
    };

    static inline const char* ToString(TextureRepeat repeat)
    {
        switch (repeat)
        {
            case TextureRepeat::Repeat:
                return "repeat";
            case TextureRepeat::MirroredRepeat:
                return "mirroredRepeat";
            case TextureRepeat::ClampToEdge:
                return "clampToEdge";
            case TextureRepeat::ClampToBorder:
                return "clampToBorder";
            default:
                C3D_ASSERT_MSG(false, "Invalid TextureRepeat");
                return "ERROR";
        }
    }

    enum TextureFlag
    {
        None = 0x0,
        /** @brief Indicates if the texture has transparency. */
        HasTransparency = 0x1,
        /** @brief Indicates if the texture is writable. */
        IsWritable = 0x2,
        /** @brief Indicates if the texture was created via wrapping vs traditional creation. */
        IsWrapped = 0x4,
        /** @brief Indicates if the texture is being used as a depth texture. */
        IsDepth = 0x8,
    };

    enum class FaceCullMode
    {
        /** @brief No faces are culled. */
        None = 0x0,
        /** @brief Only front faces are culled. */
        Front = 0x1,
        /** @brief Only back faces are culled. */
        Back = 0x2,
        /** @brief Both front and back faces are culled. */
        FrontAndBack = 0x3
    };

    enum TextureType
    {
        TextureType2D,
        TextureTypeCube,
    };
}  // namespace C3D