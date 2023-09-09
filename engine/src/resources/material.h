
#pragma once
#include "containers/string.h"
#include "math/math_types.h"
#include "texture.h"

namespace C3D
{
    constexpr auto MATERIAL_NAME_MAX_LENGTH = 256;

    enum class MaterialType
    {
        World,
        Ui
    };

    struct Material
    {
        /** @brief The id of the material. */
        u32 id = INVALID_ID;
        /** @brief The material generation. Gets incremented every time the material is changed. */
        u32 generation = INVALID_ID;
        /** @brief The material internal id. Used by the renderer backend to map to internal resources. */
        u32 internalId = INVALID_ID;
        /** @brief The material's associated shader id. */
        u32 shaderId = INVALID_ID;
        /** @brief The material name. */
        String name;
        /** @brief The material diffuse color. */
        vec4 diffuseColor;
        /** @brief The material diffuse texture map. */
        TextureMap diffuseMap;
        /** @brief The material specular texture map. */
        TextureMap specularMap;
        /** @brief The material normal texture map. */
        TextureMap normalMap;
        /** @brief The material shininess, determines how bright the specular lighting will be. */
        float shininess = 0.0f;

        /* @brief Synced to the renderer current frame number when the material has been applied that frame. */
        u32 renderFrameNumber = INVALID_ID;
    };

    struct MaterialConfig
    {
        /** @brief Name of the Material. */
        String name;
        /** @brief Name of the shader associated with this material. */
        String shaderName;
        /** @brief Indicates if this material should automatically be released when no references to it remain. */
        bool autoRelease;
        /** @brief The diffuse color of the material. */
        vec4 diffuseColor;
        /** @brief How shiny this material is. */
        float shininess;
        /** @brief The diffuse map name. */
        String diffuseMapName;
        /** @brief The specular map name. */
        String specularMapName;
        /** @brief The normal map name. */
        String normalMapName;
    };
}  // namespace C3D
