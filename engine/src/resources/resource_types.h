
#pragma once
#include "containers/string.h"
#include "core/defines.h"

namespace C3D
{
    constexpr auto BINARY_RESOURCE_FILE_MAGIC_NUMBER = 0xC3DC3D;

    // Pre-defined resource types
    enum class ResourceType : u8
    {
        None,
        Text,
        Binary,
        Image,
        Material,
        Mesh,
        Shader,
        BitmapFont,
        SystemFont,
        SimpleScene,
        Terrain,
        AudioFile,
        Custom,
        MaxValue
    };

    /** @brief The header for our proprietary resource files. */
    struct ResourceHeader
    {
        /** @brief A magic number indicating this file is a C3D binary file. */
        u32 magicNumber;
        /** @brief The type of this resource, maps to our ResourceType enum. */
        u8 resourceType;
        /** @brief The format version the resource file uses. */
        u8 version;
        /** @brief Some reserved space for future header data. */
        u16 reserved;
    };

    struct Resource
    {
        u32 loaderId = INVALID_ID;
        /** @brief The resource version. */
        u8 version = 0;
        /** @brief The name of the resource. */
        String name;
        /** @brief The full path to the resource. */
        String fullPath;
    };
}  // namespace C3D
