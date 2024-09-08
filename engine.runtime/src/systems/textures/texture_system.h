
#pragma once
#include <array>

#include "containers/hash_map.h"
#include "defines.h"
#include "logger/logger.h"
#include "resources/managers/image_manager.h"
#include "resources/textures/texture.h"
#include "string/string.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto DEFAULT_TEXTURE_NAME          = "DEFAULT";
    constexpr auto DEFAULT_ALBEDO_TEXTURE_NAME   = "DEFAULT_ALBEDO";
    constexpr auto DEFAULT_NORMAL_TEXTURE_NAME   = "DEFAULT_NORMAL";
    constexpr auto DEFAULT_COMBINED_TEXTURE_NAME = "DEFAULT_COMBINED";
    constexpr auto DEFAULT_CUBE_TEXTURE_NAME     = "DEFAULT_CUBE";
    constexpr auto DEFAULT_TERRAIN_TEXTURE_NAME  = "DEFAULT_TERRAIN";

    constexpr auto MAX_LOADING_TEXTURES = 128;

    struct TextureSystemConfig
    {
        u32 maxTextureCount;
    };

    struct TextureReference
    {
        TextureReference(bool autoRelease) : autoRelease(autoRelease) {}

        /** @brief Reference count starts at 1 since there should be at least 1 reference when we create */
        u64 referenceCount = 1;
        /** @brief RThe actual texture object */
        Texture texture;
        /** @brief RA boolean indicating if this texture should be released if referenceCount == 0 */
        bool autoRelease = false;
    };

    class C3D_API TextureSystem final : public SystemWithConfig<TextureSystemConfig>
    {
    public:
        bool OnInit(const CSONObject& config) override;
        void OnShutdown() override;

        /**
         * @brief Acquire a texture with the provided name.
         *
         * @param name The name of the texture
         * @param autoRelease A boolean indicating if we should automatically release the texture when there are no more references to it
         * @return A handle to the Texture
         */
        TextureHandle Acquire(const String& name, bool autoRelease);

        /**
         * @brief Acquire an Array Texture (multi-layer texture) with the provided name.
         * For every layer there will be one texture (as specified in the layerTextureNames).
         * All textures must be of the same dimensions for the array texture to succesfully load.
         *
         * @param name The name of the texture
         * @param layerCount The amount of layers in the texture (must be >= 1)
         * @param layerTextureNames The names of the individual textures that should make up the layers of the array texture
         * @param autoRelease A boolean indicating if we should automatically release the texture when there are no more references to it
         * @return A handle to the Array Texture
         */
        TextureHandle AcquireArray(const String& name, u32 layerCount, const DynamicArray<String>& layerTextureNames, bool autoRelease);

        /**
         * @brief Acquire a Cube Texture with the provided name.
         *
         * @param name The name of the texture
         * @param autoRelease A boolean indicating if we should automatically release the texture when there are no more references to it
         * @return A handle to the Cube Texture
         */
        TextureHandle AcquireCube(const String& name, bool autoRelease);

        /**
         * @brief Acquire a Writable Texture with the provided name.
         *
         * @param name The name of the texture
         * @param width The width of the texture
         * @param height The height of the texture
         * @param channelCount The number of channels in the texture
         * @param hasTransparency A boolean indicating if this texture will have transparency
         * @return A handle to the Writable Texture
         */
        TextureHandle AcquireWritable(const String& name, u32 width, u32 height, u8 channelCount, TextureFlagBits flags);

        /**
         * @brief Acquire a Writable Array Texture with the provided name.
         *
         * @param name The name of the texture
         * @param width The width of the texture
         * @param height The height of the texture
         * @param channelCount The number of channels in the texture
         * @param arraySize The number of layers in this array texture
         * @param hasTransparency A boolean indicating if this texture will have transparency
         * @return  A handle to the Writable Array Texture
         */
        TextureHandle AcquireArrayWritable(const String& name, u32 width, u32 height, u8 channelCount, u16 arraySize,
                                           TextureFlagBits flags);

        /**
         * @brief Release a texture by name. This decrements the reference count by 1.
         * If the reference count reaches 0 and the texture has autoRelease enabled it gets destroyed automatically.
         *
         * @param name The name of the texture you want to release
         */
        void Release(const String& name);

        /**
         * @brief Release a texture by handle. This decrements the reference count by 1.
         * If the reference count reaches - and the texture has autoRelease enabled it gets destroyed automatically.
         *
         * @param handle The handle to the texture you want to release
         */
        void Release(TextureHandle handle);

        Texture& WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, void* internalData);

        void ReleaseInternal(Texture& texture);

        bool Resize(Texture& texture, u32 width, u32 height, bool regenerateInternalData) const;
        void WriteData(Texture& texture, u32 offset, u32 size, const u8* data) const;

        /** @brief Gets the default texture. */
        TextureHandle GetDefault();
        /** @brief Gets the default diffuse (albedo) texture. */
        TextureHandle GetDefaultDiffuse();
        /** @brief Gets the default albedo (diffuse) texture. */
        TextureHandle GetDefaultAlbedo();
        /** @brief Gets the default normal texture. */
        TextureHandle GetDefaultNormal();
        /** @brief Gets the default combined(metallic, roughness and ao) texture. */
        TextureHandle GetDefaultCombined();
        /** @brief Gets the default cube texture. */
        TextureHandle GetDefaultCube();
        /** @brief Gets the default terrain texture. (which is a 12-layer texture that is build up as follows:
         * 4 materials with each a materials array-texture, a shadowmap array-texture and an irradiance cube texture) */
        TextureHandle GetDefaultTerrain();

        /**
         * @brief Checks if the provided TextureHandle references a default texture.
         *
         * @param handle The handle to the texture you want to check
         * @return True for default textures; false otherwise
         */
        bool IsDefault(TextureHandle handle) const;

        /**
         * @brief Gets a const reference to the texture data associated with the provided handle.
         *
         * @param handle The handle you want to get the texture data for
         * @return A const reference to the actual texture data
         */
        const Texture& Get(TextureHandle handle) const;

        /**
         * @brief Gets the name of the texture associated with the provided handle
         *
         * @param handle The handle you want to get the name for
         * @return const reference to the name of the texture
         */
        const String& GetName(TextureHandle handle) const;

        /**
         * @brief Indicates if the texture associated with the provided handle has transparency
         *
         * @param handle The handle to the texture
         * @return True if the texture has transparency; False otherwise
         */
        bool HasTransparency(TextureHandle handle) const;

        /**
         * @brief Gets the renderer internals for this texture.
         * NOTE: Only a specifc renderer implementation will now what to do with this data.
         *
         * @param handle The handle you want to get the internal data for
         * @return A void* to the API specific texture data
         */
        template <class Type>
        Type* GetInternals(TextureHandle handle) const
        {
#ifdef _DEBUG
            if (handle == INVALID_ID || handle >= m_textures.Size())
            {
                FATAL_LOG("Tried to get the internals of a non-existant texture.");
            }
#endif
            return static_cast<Type*>(m_textures[handle].texture.internalData);
        }

        void CreateDefaultTextures();

    private:
        TextureHandle CreateDefaultTexture(const String& name, TextureType type, u32 width, u32 height, u8 channelCount, u8* pixels,
                                           u16 arraySize = 1);

        TextureHandle CreateArrayWritable(const String& name, TextureType type, u32 width, u32 height, u8 channelCount, u16 arraySize,
                                          TextureFlagBits flags);

        bool TextureReferenceExists(const String& name) const;

        TextureReference& CreateTextureReference(const String& name, bool autoRelease);
        TextureReference& GetTextureReference(const String& name);

        void DeleteTextureReference(const String& name);

        void DestroyTexture(Texture& texture) const;

        void LoadTexture(Texture& texture);
        void LoadArrayTexture(Texture& texture, const DynamicArray<String>& layerNames);
        bool LoadCubeTexture(const CString<TEXTURE_NAME_MAX_LENGTH>* textureNames, Texture& texture) const;

        TextureHandle m_defaultTexture;
        TextureHandle m_defaultAlbedoTexture;
        TextureHandle m_defaultNormalTexture;
        TextureHandle m_defaultCombinedTexture;
        TextureHandle m_defaultCubeTexture;
        TextureHandle m_defaultTerrainTexture;

        DynamicArray<TextureReference> m_textures;
        HashMap<String, u32> m_nameToTextureIndexMap;
    };
}  // namespace C3D
