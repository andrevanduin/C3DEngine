
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/hash_table.h"
#include "containers/string.h"
#include "shader_types.h"

namespace C3D
{
    struct Texture;
    struct TextureMap;

    class C3D_API Shader
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

        /** @brief Used to sync to the renderer's frame number. To ensure we only update once per frame. */
        u64 frameNumber = INVALID_ID_U64;
        /** @brief Used to sync to the renderer's draw index. To ensure we only update once per draw. */
        u8 drawIndex = INVALID_ID_U8;

        // A pointer to the Renderer API specific data
        // This memory needs to be managed separately by the current rendering API backend
        void* apiSpecificData = nullptr;
    };
}  // namespace C3D