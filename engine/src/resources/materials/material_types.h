
#pragma once
#include <variant>

#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "resources/shaders/shader_types.h"
#include "resources/terrain/terrain_config.h"
#include "resources/textures/texture_types.h"

namespace C3D
{
    constexpr auto MATERIAL_NAME_MAX_LENGTH = 256;

    enum class MaterialType
    {
        /** @brief Invalid material type. */
        Unkown,
        Phong,
        PBR,
        UI,
        Terrain,
        Custom
    };

    static inline const char* ToString(const MaterialType type)
    {
        switch (type)
        {
            case MaterialType::Phong:
                return "phong";
            case MaterialType::PBR:
                return "pbr";
            case MaterialType::Custom:
                return "custom";
            case MaterialType::UI:
                return "ui";
            default:
                C3D_ASSERT_MSG(false, "Invalid MaterialType");
                return "ERROR";
        }
    }

    using MaterialConfigPropValue =
        std::variant<u8, i8, u16, i16, u32, i32, f32, u64, i64, f64, vec2, vec3, vec4, mat4>;

    static inline String ToString(const MaterialConfigPropValue& value)
    {
        switch (value.index())
        {
            case 0:
            {
                auto v = std::get<u8>(value);
                return String::FromFormat("{}", v);
            }
            case 1:
            {
                auto v = std::get<i8>(value);
                return String::FromFormat("{}", v);
            }
            case 2:
            {
                auto v = std::get<u16>(value);
                return String::FromFormat("{}", v);
            }
            case 3:
            {
                auto v = std::get<i16>(value);
                return String::FromFormat("{}", v);
            }
            case 4:
            {
                auto v = std::get<u32>(value);
                return String::FromFormat("{}", v);
            }
            case 5:
            {
                auto v = std::get<i32>(value);
                return String::FromFormat("{}", v);
            }
            case 6:
            {
                auto v = std::get<f32>(value);
                return String::FromFormat("{}", v);
            }
            case 7:
            {
                auto v = std::get<u64>(value);
                return String::FromFormat("{}", v);
            }
            case 8:
            {
                auto v = std::get<i64>(value);
                return String::FromFormat("{}", v);
            }
            case 9:
            {
                auto v = std::get<f64>(value);
                return String::FromFormat("{}", v);
            }
            case 10:
            {
                auto& v = std::get<vec2>(value);
                return String::FromFormat("{} {}", v.x, v.y);
            }
            case 11:
            {
                auto& v = std::get<vec3>(value);
                return String::FromFormat("{} {} {}", v.x, v.y, v.z);
            }
            case 12:
            {
                auto& v = std::get<vec4>(value);
                return String::FromFormat("{} {}", v.x, v.y, v.z, v.w);
            }
            case 13:
            {
                auto& v = std::get<mat4>(value);
                return String::FromFormat("{} {}", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10],
                                          v[11], v[12], v[13], v[14], v[15]);
            }
            default:
                C3D_ASSERT_MSG(false, "Invalid MaterialConfigPropValue");
                return "ERROR";
        }
    }

    struct MaterialConfigProp
    {
        /** @brief The name of the prop. */
        String name;
        /** @brief The type of the prop. */
        ShaderUniformType type;
        /** @brief The size of the prop. */
        u16 size;
        /** @brief The value of this prop. */
        MaterialConfigPropValue value;

        MaterialConfigProp() = default;

        MaterialConfigProp(const String& name, ShaderUniformType type, MaterialConfigPropValue value)
            : name(name), type(type), value(value)
        {}
    };

    struct MaterialConfigMap
    {
        /** @brief The name of the map. */
        String name;
        /** @brief The name of the texture. */
        String textureName;
        /** @brief The minify filter type for the texture. */
        TextureFilter minifyFilter = TextureFilter::ModeLinear;
        /** @brief The magnify filter type for the texture. */
        TextureFilter magnifyFilter = TextureFilter::ModeLinear;
        /** @brief The repeat type for the texture in the U direction. */
        TextureRepeat repeatU = TextureRepeat::Repeat;
        /** @brief The repeat type for the texture in the V direction. */
        TextureRepeat repeatV = TextureRepeat::Repeat;
        /** @brief The repeat type for the texture in the W direction. */
        TextureRepeat repeatW = TextureRepeat::Repeat;

        MaterialConfigMap() = default;

        MaterialConfigMap(const String& name, const String& textureName) : name(name), textureName(textureName) {}
    };

    struct MaterialConfig : Resource
    {
        /** @brief The type of the Material. */
        MaterialType type;
        /** @brief The name of the shader that is to be used with this Material. */
        String shaderName;
        /** @brief The configs for the properties of the Material. */
        DynamicArray<MaterialConfigProp> props;
        /** @brief The configs for the maps of the Material. */
        DynamicArray<MaterialConfigMap> maps;
        /** @brief Indicates if the Material should be automatically released when no more references to it remain.
         */
        bool autoRelease;
    };

    struct MaterialPhongProperties
    {
        /** @brief The diffuse color for the Material. */
        vec4 diffuseColor;
        vec3 padding;
        /** @brief The shininess of the Material. Determines how intense the specular lighting is. */
        f32 shininess;
    };

    struct MaterialUIProperties
    {
        /** @brief The diffuse color for the Material. */
        vec4 diffuseColor;
    };

    struct MaterialTerrainProperties
    {
        MaterialPhongProperties materials[TERRAIN_MAX_MATERIAL_COUNT];
        vec3 padding;
        u32 numMaterials;
        vec4 padding2;
    };
}  // namespace C3D