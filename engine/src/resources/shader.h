
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/hash_table.h"
#include "containers/string.h"
#include "renderer/renderer_types.h"

namespace C3D
{
    struct Texture;

    enum ShaderAttributeType : u8
    {
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

    /* @brief Configuration for an attribute. */
    struct ShaderAttributeConfig
    {
        String name;

        u8 size;
        ShaderAttributeType type;
    };

    /* @brief Configuration for a uniform. */
    struct ShaderUniformConfig
    {
        String name;

        u16 size;
        u32 location;

        ShaderUniformType type;
        ShaderScope scope = ShaderScope::None;
    };

    struct ShaderConfig
    {
        String name;

        /* @brief The face cull mode to be used. Default is BACK if not supplied. */
        FaceCullMode cullMode;

        DynamicArray<ShaderAttributeConfig> attributes;
        DynamicArray<ShaderUniformConfig> uniforms;

        DynamicArray<ShaderStage> stages;

        DynamicArray<String> stageNames;
        DynamicArray<String> stageFileNames;

        ShaderTopology topology = ShaderTopology::Triangles;

        // TODO: Convert these bools to flags to save space
        /* @brief Indicates if depth testing should be done by this shader. */
        bool depthTest;
        /* @brief Indicates if depth writing should be done by this shader (this is ignored if depthTest = false). */
        bool depthWrite;
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

    enum ShaderFlags
    {
        ShaderFlagNone       = 0x0,
        ShaderFlagDepthTest  = 0x1,
        ShaderFlagDepthWrite = 0x2,
    };
    typedef u32 ShaderFlagBits;

    class Shader
    {
    public:
        u16 GetUniformIndex(const char* uniformName);

        u32 id = INVALID_ID;
        String name;

        ShaderFlagBits flags = ShaderFlagNone;

        u64 requiredUboAlignment = 0;
        // A running total of the size of the global uniform buffer object
        u64 globalUboSize   = 0;
        u64 globalUboStride = 0;
        u64 globalUboOffset = 0;

        // A running total of the size of the instance uniform buffer object
        u64 uboSize         = 0;
        u64 uboStride       = 0;
        u16 attributeStride = 0;

        DynamicArray<TextureMap*> globalTextureMaps;
        u64 instanceTextureCount = 0;

        ShaderScope boundScope = ShaderScope::None;
        u32 boundInstanceId    = INVALID_ID;
        u32 boundUboOffset     = 0;

        HashMap<String, ShaderUniform> uniforms;
        DynamicArray<ShaderAttribute> attributes;

        u64 pushConstantSize = 0;
        // Note: this is hard-coded because the vulkan spec only guarantees a minimum 128bytes stride
        // The drivers might allocate more but this is not guaranteed on all video cards
        u64 pushConstantStride    = 128;
        u8 pushConstantRangeCount = 0;
        Range pushConstantRanges[32];

        ShaderState state = ShaderState::Uninitialized;

        // @brief used to ensure the shaders globals are only updated once per frame.
        u64 frameNumber = INVALID_ID_U64;

        // A pointer to the Renderer API specific data
        // This memory needs to be managed separately by the current rendering API backend
        void* apiSpecificData = nullptr;
    };
}  // namespace C3D