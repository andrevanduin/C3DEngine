
#pragma once
#include <array>

#include "containers/hash_map.h"
#include "containers/string.h"
#include "core/defines.h"
#include "core/logger.h"
#include "resources/loaders/image_loader.h"
#include "resources/textures/texture.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto DEFAULT_TEXTURE_NAME          = "default";
    constexpr auto DEFAULT_ALBEDO_TEXTURE_NAME   = "defaultAlbedo";
    constexpr auto DEFAULT_NORMAL_TEXTURE_NAME   = "defaultNormal";
    constexpr auto DEFAULT_COMBINED_TEXTURE_NAME = "defaultCombined";
    constexpr auto DEFAULT_CUBE_TEXTURE_NAME     = "defaultCube";
    constexpr auto DEFAULT_TERRAIN_TEXTURE_NAME  = "defaultTerrain";

    constexpr auto MAX_LOADING_TEXTURES = 128;

    struct TextureSystemConfig
    {
        u32 maxTextureCount;
    };

    struct TextureReference
    {
        TextureReference(bool autoRelease) : autoRelease(autoRelease) {}

        u64 referenceCount = 0;
        Texture texture;
        bool autoRelease = false;
    };

    /** @brief Structure to hold a texture that is currently being loaded. */
    struct LoadingTexture
    {
        u32 id = INVALID_ID;
        String resourceName;
        Texture* outTexture = nullptr;
        Texture tempTexture;
        u32 currentGeneration = INVALID_ID;
        Image imageResource;
    };

    struct LoadingArrayTexture
    {
        u32 id = INVALID_ID;
        String name;

        u32 layerCount = 0;
        DynamicArray<String> layerNames;

        Texture* outTexture = nullptr;
        Texture tempTexture;

        u64 dataBlockSize = 0;
        u8* dataBlock     = nullptr;

        u32 currentGeneration = INVALID_ID;
        u32 resultCode;

        Image resource;
    };

    class C3D_API TextureSystem final : public SystemWithConfig<TextureSystemConfig>
    {
    public:
        bool OnInit(const TextureSystemConfig& config) override;
        void OnShutdown() override;

        /**
         * @brief Acquire a texture with the provided name.
         *
         * @param name The name of the texture
         * @param autoRelease A boolean indicating if we should automatically release the texture when there are no more references to it
         * @return A pointer to the acquired texture
         */
        Texture* Acquire(const char* name, bool autoRelease);

        /**
         * @brief Acquire an array texture (multi-layer texture) with the provided name.
         * For every layer there will be one texture (as specified in the layerTextureNames).
         * All textures must be of the same dimensions for the array texture to succesfully load.
         *
         * @param name The name of the texture
         * @param layerCount The amount of layers in the texture (must be >= 1)
         * @param layerTextureNames The names of the individual textures that should make up the layers of the array texture
         * @param autoRelease A boolean indicating if we should automatically release the texture when there are no more references to it
         * @return A pointer to the acquired array texture
         */
        Texture* AcquireArray(const char* name, u32 layerCount, const DynamicArray<String>& layerTextureNames, bool autoRelease);

        Texture* AcquireCube(const char* name, bool autoRelease);

        Texture* AcquireWritable(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency);
        Texture* AcquireArrayWritable(const char* name, u32 width, u32 height, u8 channelCount, u16 arraySize, TextureType type,
                                      bool hasTransparency);

        void Release(const String& name);

        void WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency, bool isWritable,
                          bool registerTexture, void* internalData, Texture* outTexture);

        static bool SetInternal(Texture* t, void* internalData);

        bool Resize(Texture* t, u32 width, u32 height, bool regenerateInternalData) const;
        bool WriteData(Texture* t, u32 offset, u32 size, const u8* data) const;

        /** @brief Gets the default texture. */
        Texture* GetDefault();
        /** @brief Gets the default diffuse (albedo) texture. */
        Texture* GetDefaultDiffuse();
        /** @brief Gets the default albedo (diffuse) texture. */
        Texture* GetDefaultAlbedo();
        /** @brief Gets the default normal texture. */
        Texture* GetDefaultNormal();
        /** @brief Gets the default combined(metallic, roughness and ao) texture. */
        Texture* GetDefaultCombined();
        /** @brief Gets the default cube texture. */
        Texture* GetDefaultCube();
        /** @brief Gets the default terrain texture. (which is a 12-layer texture that is build up as follows:
         * 4 materials with each a materials array-texture, a shadowmap array-texture and an irradiance cube texture) */
        Texture* GetDefaultTerrain();

        bool IsDefault(const Texture* texture) const;

        bool CreateDefaultTextures();

    private:
        void DestroyDefaultTextures();

        bool CreateTexture(Texture& texture, TextureType type, u32 width, u32 height, u8 channelCount, u16 arraySize,
                           const DynamicArray<String>& layerTextureNames, bool isWritable, bool skipLoad);
        void DestroyTexture(Texture* texture) const;

        bool LoadTexture(Texture& texture, const DynamicArray<String>& layerNames);
        bool LoadCubeTextures(const CString<TEXTURE_NAME_MAX_LENGTH>* textureNames, Texture& texture) const;

        bool ProcessTextureReference(const char* name, i8 referenceDiff, bool autoRelease, u32& outTextureId, bool& outNeedsCreation);

        bool LoadTextureEntryPoint(u32 loadingTextureIndex);
        bool LoadLayeredTextureEntryPoint(u32 loadingTextureIndex);

        void LoadTextureSuccess(u32 loadingTextureIndex);
        void LoadLayeredTextureSuccess(u32 loadingTextureIndex);

        void CleanupLoadingTexture(u32 loadingTextureIndex);
        void CleanupLoadingLayeredTexture(u32 loadingTextureIndex);

        Texture m_defaultTexture;
        Texture m_defaultAlbedoTexture;
        Texture m_defaultNormalTexture;
        Texture m_defaultCombinedTexture;
        Texture m_defaultCubeTexture;
        Texture m_defaultTerrainTexture;

        DynamicArray<TextureReference> m_textures;
        HashMap<String, u32> m_nameToTextureIndexMap;

        Array<LoadingTexture, MAX_LOADING_TEXTURES> m_loadingTextures;
        Array<LoadingArrayTexture, MAX_LOADING_TEXTURES> m_loadingArrayTextures;
    };
}  // namespace C3D
