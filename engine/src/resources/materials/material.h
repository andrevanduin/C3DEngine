
#pragma once
#include <variant>

#include "containers/string.h"
#include "math/math_types.h"
#include "resources/textures/texture_map.h"

namespace C3D
{
    struct Material
    {
        /** @brief The id of the Material. */
        u32 id = INVALID_ID;
        /** @brief The Material generation. Gets incremented every time the material is changed. */
        u32 generation = INVALID_ID;
        /** @brief The Material internal id. Used by the renderer backend to map to internal resources. */
        u32 internalId = INVALID_ID;
        /** @brief The shader id that is associated with this Material. */
        u32 shaderId = INVALID_ID;
        /** @brief The Material name. */
        String name;
        /** @brief The Material type. */
        MaterialType type;
        /** @brief The texture maps associated with this Material. */
        DynamicArray<TextureMap> maps;
        /** @brief An explicit irradiance texture to use for this material. If not set this material will use the scene's irradiance map. */
        Texture* irradianceTexture = nullptr;
        /** @brief The size of the properties structure for this Material. */
        u32 propertiesSize = 0;
        /** @brief The properties associated with this Material. */
        void* properties = nullptr;
        /** @brief Synced to the renderer current frame number when the material has been applied that frame. */
        u64 renderFrameNumber = INVALID_ID_U64;
        u64 renderDrawIndex   = INVALID_ID_U64;
    };
}  // namespace C3D
