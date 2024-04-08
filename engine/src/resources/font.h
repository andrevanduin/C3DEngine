
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "resources/textures/texture_map.h"

namespace C3D
{
    constexpr auto FONT_DATA_FACE_MAX_LENGTH = 256;

    struct FontGlyph
    {
        i32 codepoint;
        u16 x;
        u16 y;
        u16 width;
        u16 height;
        i16 xOffset;
        i16 yOffset;
        i16 xAdvance;
        u8 pageId;
    };

    struct FontKerning
    {
        i32 codepoint0;
        i32 codepoint1;
        i16 amount;
    };

    enum class FontType : u8
    {
        Unknown,
        Bitmap,
        System,
    };

    struct FontData
    {
        FontType type;
        String face;
        u32 size;
        i32 lineHeight;
        i32 baseline;
        u32 atlasSizeX;
        u32 atlasSizeY;
        TextureMap atlas;
        DynamicArray<FontGlyph> glyphs;
        DynamicArray<FontKerning> kernings;
        f32 tabXAdvance;
        u32 internalDataSize;
        void* internalData;
    };

    struct BitmapFontPage
    {
        i8 id;
        String file;
    };

    struct SystemFontFace
    {
        String name;
    };

    using FontHandle = UUID;
}  // namespace C3D