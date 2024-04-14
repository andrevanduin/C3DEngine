
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "renderer/renderer_types.h"
#include "resources/resource_types.h"
#include "resources/textures/texture_types.h"

namespace C3D
{
    enum ShaderAttributeType : u8
    {
        Attribute_Unknown,
        Attribute_Float32,
        Attribute_Float32_2,
        Attribute_Float32_3,
        Attribute_Float32_4,
        Attribute_Matrix4,
        Attribute_Int8,
        Attribute_UInt8,
        Attribute_Int16,
        Attribute_UInt16,
        Attribute_Int32,
        Attribute_UInt32
    };

    enum ShaderUniformType : u8
    {
        Uniform_Unknown,
        Uniform_Float32,
        Uniform_Float32_2,
        Uniform_Float32_3,
        Uniform_Float32_4,
        Uniform_Int8,
        Uniform_UInt8,
        Uniform_Int16,
        Uniform_UInt16,
        Uniform_Int32,
        Uniform_UInt32,
        Uniform_Matrix4,
        Uniform_Sampler,
        Uniform_Custom = 255
    };

    static inline const char* ToString(ShaderUniformType type)
    {
        switch (type)
        {
            case ShaderUniformType::Uniform_Float32:
                return "f32";
            case ShaderUniformType::Uniform_Float32_2:
                return "vec2";
            case ShaderUniformType::Uniform_Float32_3:
                return "vec3";
            case ShaderUniformType::Uniform_Float32_4:
                return "vec4";
            case ShaderUniformType::Uniform_Int8:
                return "i8";
            case ShaderUniformType::Uniform_UInt8:
                return "u8";
            case ShaderUniformType::Uniform_Int16:
                return "i16";
            case ShaderUniformType::Uniform_UInt16:
                return "u16";
            case ShaderUniformType::Uniform_Int32:
                return "i32";
            case ShaderUniformType::Uniform_UInt32:
                return "u32";
            case ShaderUniformType::Uniform_Matrix4:
                return "mat4";
            case ShaderUniformType::Uniform_Sampler:
                return "sampler";
            case ShaderUniformType::Uniform_Custom:
                return "custom";
            default:
                C3D_ASSERT_MSG(false, "Invalid ShaderUniformType");
                return "ERROR";
        }
    }

    /* @brief the different possible scopes in a shader. */
    enum class ShaderScope
    {
        None,
        Global,
        Instance,
        Local,
    };

    enum class ShaderTopology
    {
        Points,
        Lines,
        Triangles
    };

    /** @brief Configuration for an attribute. */
    struct ShaderAttributeConfig
    {
        String name;

        u8 size;
        ShaderAttributeType type;
    };

    /** @brief Configuration for a uniform. */
    struct ShaderUniformConfig
    {
        String name;

        u16 size;
        u32 location;

        ShaderUniformType type;
        ShaderScope scope = ShaderScope::None;
    };

    enum ShaderFlags
    {
        ShaderFlagNone         = 0x0,
        ShaderFlagDepthTest    = 0x01,
        ShaderFlagDepthWrite   = 0x02,
        ShaderFlagStencilTest  = 0x04,
        ShaderFlagStencilWrite = 0x08,
        ShaderFlagWireframe    = 0x10,
    };
    typedef u32 ShaderFlagBits;

    struct ShaderConfig final : Resource
    {
        String name;

        /** @brief The face cull mode to be used. Default is BACK if not supplied. */
        FaceCullMode cullMode;

        DynamicArray<ShaderAttributeConfig> attributes;
        DynamicArray<ShaderUniformConfig> uniforms;

        DynamicArray<ShaderStage> stages;

        DynamicArray<String> stageNames;
        DynamicArray<String> stageFileNames;

        /** @brief The types of toplogy that this shader should support. */
        u32 topologyTypes = 0;

        /** @brief The flags that need to be set*/
        ShaderFlagBits flags = ShaderFlagNone;
    };

    enum class ShaderState : u8
    {
        NotCreated,
        Uninitialized,
        Initialized
    };

    struct ShaderUniform
    {
        u64 offset;
        u16 location;
        u16 index;
        u16 size;
        u8 setIndex;
        ShaderScope scope;
        ShaderUniformType type;
    };

    struct ShaderAttribute
    {
        String name;
        ShaderAttributeType type;
        u32 size;
    };
}  // namespace C3D