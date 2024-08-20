
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
        u16 GetUniformIndex(const String& uniformName) const;

        /** @brief The id for this shader. */
        u32 id = INVALID_ID;
        /** @brief The name of this shader. */
        String name;
        /** @brief The relevant flags for this shader. */
        ShaderFlagBits flags = ShaderFlagNone;
        /** @brief The types of topology used by this shader and it's pipelines. */
        u32 topologyTypes;

        /** @brief A boolean indicating if this shader has wireframe rendering enabled. */
        bool wireframeEnabled = false;

        /** @brief The amount of bytes that are required for UBO alignment. This is used to determine stride which is how much the UBOs are
         * spaced out in the buffer. For example a required aligment of 256 means that the stride must be a multiple of 256. */
        u64 requiredUboAlignment = 0;
        /** @brief A running total of the size of the global uniform buffer object. */
        u64 globalUboSize = 0;
        /** @brief The stride of the global uniform buffer object. */
        u64 globalUboStride = 0;
        /** @brief The offset from the beginning in bytes for the global UBO. */
        u64 globalUboOffset = 0;

        /** @brief The size of the instance UBO. */
        u64 uboSize = 0;
        /** @brief The stride of the instance UBO. */
        u64 uboStride = 0;

        /** @brief The size of the local UBO. */
        u64 localUboSize = 0;
        /** @brief The stride of the instance UBO. */
        u64 localUboStride = 0;
        /** @brief The offset from the beginning in bytes for the local UBO. */
        u64 localUboOffset = 0;

        /** @brief An array of global texture map pointers. */
        DynamicArray<TextureMap*> globalTextureMaps;
        /** @brief The number of instance textures. */
        u16 instanceTextureCount = 0;
        /** @brief The currently bound scope for this shader. */
        ShaderScope boundScope = ShaderScope::None;
        /** @brief The id for the currently bound instance. */
        u32 boundInstanceId = INVALID_ID;
        /** @brief The currently bound instance's UBO offset. */
        u32 boundUboOffset = 0;
        /** @brief A HashMap that maps the name of a uniform to it's index in the uniform array. */
        HashMap<String, u64> uniformNameToIndexMap;
        /** @brief An array of our Shader's actual uniforms. */
        DynamicArray<ShaderUniform> uniforms;

        /** @brief The number of global non-sampler uniforms. */
        u8 globalUniformCount = 0;
        /** @brief The number of global sampler uniforms. */
        u8 globalUniformSamplerCount = 0;
        /** @brief An array to keep track if the indices of the uniforms used for global samplers. */
        DynamicArray<u16> globalSamplers;
        /** @brief The number of instance non-sampler uniforms. */
        u8 instanceUniformCount = 0;
        /** @brief The number of instance sampler uniforms. */
        u8 instanceUniformSamplerCount = 0;
        /** @brief An array to keep track of the indices of the uniforms used for instance samplers. */
        DynamicArray<u16> instanceSamplers;
        /** @brief The number of local non-sampler uniforms. */
        u8 localUniformCount = 0;

        /** @brief An array of this shader's attributes. */
        DynamicArray<ShaderAttribute> attributes;
        /** @brief The stride of the attributes. */
        u16 attributeStride = 0;

        /** @brief The internal state of the Shader. */
        ShaderState state = ShaderState::Uninitialized;

        /** @brief Used to sync to the renderer's frame number. To ensure we only update once per frame. */
        u64 frameNumber = INVALID_ID_U64;
        /** @brief Used to sync to the renderer's draw index. To ensure we only update once per draw. */
        u8 drawIndex = INVALID_ID_U8;

        /** @brief An array of per-stage config. */
        DynamicArray<ShaderStageConfig> stageConfigs;

#ifdef _DEBUG
        /** @brief An array of watch id's for the files associated with this shader (used for hot-reloading). */
        DynamicArray<FileWatchId> moduleWatchIds;
#endif

        // A pointer to the Renderer API specific data
        // This memory needs to be managed separately by the current rendering API backend
        void* apiSpecificData = nullptr;
    };
}  // namespace C3D