
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
        Uniform_Sampler1D,
        Uniform_Sampler2D,
        Uniform_Sampler3D,
        Uniform_SamplerCube,
        Uniform_Sampler1DArray,
        Uniform_Sampler2DArray,
        Uniform_SamplerCubeArray,
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
            case ShaderUniformType::Uniform_Sampler1D:
                return "Sampler1D";
            case ShaderUniformType::Uniform_Sampler2D:
                return "Sampler2D";
            case ShaderUniformType::Uniform_Sampler3D:
                return "Sampler3D";
            case ShaderUniformType::Uniform_SamplerCube:
                return "SamplerCube";
            case ShaderUniformType::Uniform_Sampler1DArray:
                return "Sampler1DArray";
            case ShaderUniformType::Uniform_Sampler2DArray:
                return "Sampler2DArray";
            case ShaderUniformType::Uniform_SamplerCubeArray:
                return "SamplerCubeArray";
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
        None     = -1,
        Global   = 0,
        Instance = 1,
        Local    = 2,
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
        /** @brief The name of the attribute. */
        String name;
        /** @brief The size of the attribute. */
        u8 size;
        /** @brief The type of the attribute. */
        ShaderAttributeType type;
    };

    /** @brief Configuration for a uniform. */
    struct ShaderUniformConfig
    {
        /** @brief The name of the uniform. */
        String name;
        /** @brief The size of the uniform. If the uniform is an array this is the per-element size. */
        u16 size = 0;
        /** @brief The location of the uniform. */
        u32 location = INVALID_ID;
        /** @brief The type of this uniform (vec2, sampler2D etc.). */
        ShaderUniformType type;
        /** @brief The array length for this uniform (non-array types will always be 1). */
        u8 arrayLength = 1;
        /** @brief The scope of this uniform (global, instance or local). */
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
        /** @brief The types of toplogy for the shader pipeline. */
        u32 topologyTypes;
        /** @brief A list of attributes used by this shader. */
        DynamicArray<ShaderAttributeConfig> attributes;
        /** @brief A list of uniforms used by this shader. */
        DynamicArray<ShaderUniformConfig> uniforms;
        /** @brief The per-stage config for this shader. */
        DynamicArray<ShaderStageConfig> stageConfigs;
        /** @brief The maximum number of instances allowed for this shader. */
        u32 maxInstances = 1;
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
        u8 arrayLength;
        ShaderScope scope;
        ShaderUniformType type;
    };

    struct ShaderAttribute
    {
        String name;
        ShaderAttributeType type;
        u32 size;
    };

    struct ShaderInstanceUniformTextureConfig
    {
        /** @brief The location of the uniform to map to. */
        u16 uniformLocation = INVALID_ID_U16;
        /** @brief The number of texture map pointers mapped to the uniform. */
        u8 textureMapCount = 0;
        /** @brief An array of texture map pointers to be mapped to the uniform. */
        TextureMap** textureMaps;
    };

    struct ShaderInstanceResourceConfig
    {
        u32 uniformConfigCount                             = 0;
        ShaderInstanceUniformTextureConfig* uniformConfigs = nullptr;
    };
}  // namespace C3D