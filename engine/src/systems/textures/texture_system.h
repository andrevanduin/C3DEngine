
#pragma once
#include <array>

#include "containers/hash_table.h"
#include "core/defines.h"
#include "core/logger.h"
#include "resources/loaders/image_loader.h"
#include "resources/texture.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto DEFAULT_TEXTURE_NAME = "default";
    constexpr auto DEFAULT_DIFFUSE_TEXTURE_NAME = "defaultDiffuse";
    constexpr auto DEFAULT_SPECULAR_TEXTURE_NAME = "defaultSpecular";
    constexpr auto DEFAULT_NORMAL_TEXTURE_NAME = "defaultNormal";
    constexpr auto MAX_LOADING_TEXTURES = 128;

    struct TextureSystemConfig
    {
        u32 maxTextureCount;
    };

    struct TextureReference
    {
        u64 referenceCount = 0;
        u32 handle = INVALID_ID;
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
        ImageResource imageResource;
    };

    class C3D_API TextureSystem final : public SystemWithConfig<TextureSystemConfig>
    {
    public:
        TextureSystem(const SystemManager* pSystemsManager);

        bool Init(const TextureSystemConfig& config) override;
        void Shutdown() override;

        Texture* Acquire(const char* name, bool autoRelease);
        Texture* AcquireCube(const char* name, bool autoRelease);
        Texture* AcquireWritable(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency);

        void Release(const char* name);

        void WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, bool hasTransparency,
                          bool isWritable, bool registerTexture, void* internalData, Texture* outTexture);

        static bool SetInternal(Texture* t, void* internalData);

        bool Resize(Texture* t, u32 width, u32 height, bool regenerateInternalData) const;
        bool WriteData(Texture* t, u32 offset, u32 size, const u8* data) const;

        Texture* GetDefault();
        Texture* GetDefaultDiffuse();
        Texture* GetDefaultSpecular();
        Texture* GetDefaultNormal();

        bool CreateDefaultTextures();

    private:
        void DestroyDefaultTextures();

        bool LoadTexture(const char* name, Texture* texture);
        bool LoadCubeTextures(const char* name, const std::array<CString<TEXTURE_NAME_MAX_LENGTH>, 6>& textureNames,
                              Texture* texture) const;

        void DestroyTexture(Texture* texture) const;

        bool ProcessTextureReference(const char* name, TextureType type, i8 referenceDiff, bool autoRelease,
                                     bool skipLoad, u32* outTextureId);

        bool LoadJobEntryPoint(u32 loadingTextureIndex);
        void LoadJobSuccess(u32 loadingTextureIndex);

        void CleanupLoadingTexture(u32 loadingTextureIndex);

        bool m_initialized;

        Texture m_defaultTexture;
        Texture m_defaultDiffuseTexture;
        Texture m_defaultSpecularTexture;
        Texture m_defaultNormalTexture;

        // TODO: Replace with HashMap
        Texture* m_registeredTextures;
        HashTable<TextureReference> m_registeredTextureTable;

        Array<LoadingTexture, MAX_LOADING_TEXTURES> m_loadingTextures;
    };
}  // namespace C3D
