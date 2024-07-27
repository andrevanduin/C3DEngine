
#include "texture_system.h"

#include "core/engine.h"
#include "core/jobs/job.h"
#include "core/logger.h"
#include "core/string_utils.h"
#include "renderer/renderer_frontend.h"
#include "resources/loaders/image_loader.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "TEXTURE_SYSTEM";

    bool TextureSystem::OnInit(const TextureSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxTextureCount == 0)
        {
            ERROR_LOG("config.maxTextureCount must be > 0.");
            return false;
        }

        m_config = config;

        // Ensure that we have enough space for all our textures
        m_registeredTextures.Create(config.maxTextureCount);

        CreateDefaultTextures();

        m_initialized = true;
        return true;
    }

    void TextureSystem::OnShutdown()
    {
        INFO_LOG("Destroying all loaded textures.");
        for (auto& ref : m_registeredTextures)
        {
            if (ref.texture.generation != INVALID_ID)
            {
                Renderer.DestroyTexture(&ref.texture);
            }
        }

        // Free the memory that was storing all the textures
        m_registeredTextures.Destroy();

        INFO_LOG("Destroying default textures.");
        DestroyDefaultTextures();
    }

    Texture* TextureSystem::Acquire(const char* name, const bool autoRelease)
    {
        if (StringUtils::IEquals(name, DEFAULT_TEXTURE_NAME))
        {
            WARN_LOG("Called for '{}' texture. Use GetDefault() for this.", DEFAULT_TEXTURE_NAME);
            return &m_defaultTexture;
        }

        if (StringUtils::IEquals(name, DEFAULT_ALBEDO_TEXTURE_NAME))
        {
            WARN_LOG("Called for '{}' texture. Use GetDefault() for this.", DEFAULT_ALBEDO_TEXTURE_NAME);
            return &m_defaultAlbedoTexture;
        }

        if (StringUtils::IEquals(name, DEFAULT_NORMAL_TEXTURE_NAME))
        {
            WARN_LOG("Called for '{}' texture. Use GetDefault() for this.", DEFAULT_NORMAL_TEXTURE_NAME);
            return &m_defaultNormalTexture;
        }

        if (StringUtils::IEquals(name, DEFAULT_COMBINED_TEXTURE_NAME))
        {
            WARN_LOG("Called for '{}' texture. Use GetDefault() for this.", DEFAULT_COMBINED_TEXTURE_NAME);
            return &m_defaultCombinedTexture;
        }

        bool needsCreation = false;
        u32 id             = INVALID_ID;

        if (!ProcessTextureReference(name, 1, autoRelease, id, needsCreation))
        {
            ERROR_LOG("Failed to obtain texture id.");
            return nullptr;
        }

        auto& ref     = m_registeredTextures.GetByIndex(id);
        auto& texture = ref.texture;

        if (needsCreation)
        {
            DynamicArray<String> layerTextureNames;
            if (!CreateTexture(texture, TextureType2D, 0, 0, 0, 1, layerTextureNames, false, false))
            {
                ERROR_LOG("Failed to create new cube texture.");
                return nullptr;
            }
        }

        return &texture;
    }

    Texture* TextureSystem::AcquireArray(const char* name, u32 layerCount, const DynamicArray<String>& layerTextureNames, bool autoRelease)
    {
        if (layerCount < 1)
        {
            ERROR_LOG("A texture must contain at least 1 layer.");
            return nullptr;
        }

        bool needsCreation = false;
        u32 id             = INVALID_ID;

        if (!ProcessTextureReference(name, 1, autoRelease, id, needsCreation))
        {
            ERROR_LOG("Failed to obtain texture id.");
            return nullptr;
        }

        auto& ref     = m_registeredTextures.GetByIndex(id);
        auto& texture = ref.texture;

        // Create the texture if needed
        if (needsCreation)
        {
            if (!CreateTexture(texture, TextureType2DArray, 0, 0, 0, layerCount, layerTextureNames, false, false))
            {
                ERROR_LOG("Failed to create new texture.");
                return nullptr;
            }
        }

        return &texture;
    }

    Texture* TextureSystem::AcquireCube(const char* name, const bool autoRelease)
    {
        // If the default texture is requested we return it. But we warn about it since it should be
        // retrieved with GetDefault()
        if (StringUtils::IEquals(name, DEFAULT_CUBE_TEXTURE_NAME))
        {
            WARN_LOG("Called for '{}' texture. Use GetDefault() for this.", DEFAULT_CUBE_TEXTURE_NAME);
            return &m_defaultCubeTexture;
        }

        u32 id             = INVALID_ID;
        bool needsCreation = false;
        if (!ProcessTextureReference(name, 1, autoRelease, id, needsCreation))
        {
            ERROR_LOG("Failed to obtain a new texture id.");
            return nullptr;
        }

        auto& ref     = m_registeredTextures.GetByIndex(id);
        auto& texture = ref.texture;

        if (needsCreation)
        {
            DynamicArray<String> layerTextureNames;
            if (!CreateTexture(texture, TextureTypeCube, 0, 0, 0, 6, layerTextureNames, false, false))
            {
                ERROR_LOG("Failed to create new cube texture.");
                return nullptr;
            }
        }

        return &texture;
    }

    Texture* TextureSystem::AcquireWritable(const char* name, const u32 width, const u32 height, const u8 channelCount,
                                            const bool hasTransparency)
    {
        return AcquireArrayWritable(name, width, height, channelCount, 1, TextureType2D, hasTransparency);
    }

    Texture* TextureSystem::AcquireArrayWritable(const char* name, u32 width, u32 height, u8 channelCount, u16 arraySize, TextureType type,
                                                 bool hasTransparency)
    {
        u32 id             = INVALID_ID;
        bool needsCreation = false;

        if (!ProcessTextureReference(name, 1, false, id, needsCreation))
        {
            ERROR_LOG("Failed to obtain new texture id.");
            return nullptr;
        }

        auto& ref     = m_registeredTextures.GetByIndex(id);
        auto& texture = ref.texture;

        // Create the texture if it's needed
        if (needsCreation)
        {
            DynamicArray<String> layerTextureNames;  // Empty array since we will have no layers
            if (!CreateTexture(texture, type, width, height, channelCount, arraySize, layerTextureNames, true, true))
            {
                ERROR_LOG("Failed to create new texture.");
                return nullptr;
            }
        }

        texture.flags |= hasTransparency ? TextureFlag::HasTransparency : 0;
        return &texture;
    }

    void TextureSystem::Release(const String& name)
    {
        if (name == DEFAULT_TEXTURE_NAME || name == DEFAULT_ALBEDO_TEXTURE_NAME || name == DEFAULT_COMBINED_TEXTURE_NAME ||
            name == DEFAULT_CUBE_TEXTURE_NAME || name == DEFAULT_TERRAIN_TEXTURE_NAME || name == DEFAULT_NORMAL_TEXTURE_NAME)
        {
            WARN_LOG("Tried to release '{}'. This happens on shutdown automatically.", DEFAULT_TEXTURE_NAME);
            return;
        }

        u32 id             = INVALID_ID;
        bool needsCreation = false;
        if (!ProcessTextureReference(name.Data(), -1, false, id, needsCreation))
        {
            ERROR_LOG("Failed to release texture: '{}'.", name);
        }
    }

    void TextureSystem::WrapInternal(const char* name, const u32 width, const u32 height, const u8 channelCount, const bool hasTransparency,
                                     const bool isWritable, const bool registerTexture, void* internalData, Texture* outTexture)
    {
        u32 id             = INVALID_ID;
        bool needsCreation = false;
        Texture* t;

        if (registerTexture)
        {
            // NOTE: Wrapped textures are never auto-released because it means that their
            // resources are created and managed somewhere within the renderer internals.
            if (!ProcessTextureReference(name, 1, false, id, needsCreation))
            {
                ERROR_LOG("Failed to obtain a new texture id.");
                return;
            }
            t = &m_registeredTextures.GetByIndex(id).texture;
        }
        else
        {
            if (outTexture)
            {
                t = outTexture;
            }
            else
            {
                t = Memory.New<Texture>(MemoryType::Texture);
            }
        }

        auto flags = hasTransparency ? HasTransparency : 0;
        flags |= isWritable ? IsWritable : 0;
        flags |= IsWrapped;

        t->Set(TextureType2D, name, width, height, channelCount, flags, internalData);
    }

    bool TextureSystem::SetInternal(Texture* t, void* internalData)
    {
        if (t)
        {
            t->internalData = internalData;
            t->generation++;
            return true;
        }
        return false;
    }

    bool TextureSystem::Resize(Texture* t, const u32 width, const u32 height, const bool regenerateInternalData) const
    {
        if (t)
        {
            if (!t->IsWritable())
            {
                WARN_LOG("Should not be called on textures that are not writable.");
                return false;
            }

            t->width  = width;
            t->height = height;

            if (!t->IsWrapped() && regenerateInternalData)
            {
                Renderer.ResizeTexture(t, width, height);
                return false;
            }

            t->generation++;
            return true;
        }
        return false;
    }

    bool TextureSystem::WriteData(Texture* t, const u32 offset, const u32 size, const u8* data) const
    {
        if (t)
        {
            Renderer.WriteDataToTexture(t, offset, size, data);
            return true;
        }
        return false;
    }

    Texture* TextureSystem::GetDefault()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultTexture;
    }

    Texture* TextureSystem::GetDefaultDiffuse()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultAlbedoTexture;
    }

    Texture* TextureSystem::GetDefaultAlbedo()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultAlbedoTexture;
    }

    Texture* TextureSystem::GetDefaultNormal()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultNormalTexture;
    }

    Texture* TextureSystem::GetDefaultCombined()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultCombinedTexture;
    }

    Texture* TextureSystem::GetDefaultCube()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultCubeTexture;
    }

    Texture* TextureSystem::GetDefaultTerrain()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning nullptr.");
            return nullptr;
        }
        return &m_defaultTerrainTexture;
    }

    bool TextureSystem::IsDefault(const Texture* t) const
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning false.");
            return false;
        }
        return (t == &m_defaultTexture) || (t == &m_defaultAlbedoTexture) || (t == &m_defaultNormalTexture) ||
               (t == &m_defaultCombinedTexture) || (t == &m_defaultCubeTexture);
    }

    bool TextureSystem::CreateDefaultTextures()
    {
        // NOTE: Create a default texture, a 16x16 blue/white checkerboard pattern
        constexpr u32 textureDimensions = 16;
        constexpr u32 channels          = 4;
        constexpr u32 pixelCount        = textureDimensions * textureDimensions;
        constexpr u32 totalSize         = pixelCount * channels;

        u8 pixels[totalSize];
        u8 albedoPixels[totalSize];
        u8 normalPixels[totalSize];
        u8 combinedPixels[totalSize];

        {
            TRACE("Create default texture...");

            std::memset(pixels, 255, sizeof(u8) * pixelCount * channels);

            for (u64 row = 0; row < textureDimensions; row++)
            {
                for (u64 col = 0; col < textureDimensions; col++)
                {
                    const u64 index        = (row * textureDimensions) + col;
                    const u64 indexChannel = index * channels;
                    if (row % 2)
                    {
                        if (col % 2)
                        {
                            pixels[indexChannel + 0] = 0;
                            pixels[indexChannel + 1] = 0;
                        }
                    }
                    else
                    {
                        if (!(col % 2))
                        {
                            pixels[indexChannel + 0] = 0;
                            pixels[indexChannel + 1] = 0;
                        }
                    }
                }
            }

            m_defaultTexture = Texture(DEFAULT_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions, channels);
            Renderer.CreateTexture(pixels, &m_defaultTexture);
            // Manually set the texture generation to invalid since this is the default texture
            m_defaultTexture.generation = INVALID_ID;
        }

        {
            // Albedo texture
            TRACE("Create default albedo texture...");
            // Default albedo texture is all white
            std::memset(albedoPixels, 255, sizeof(u8) * pixelCount * channels);  // Default albedo map is all white

            m_defaultAlbedoTexture = Texture(DEFAULT_ALBEDO_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions, channels);
            Renderer.CreateTexture(albedoPixels, &m_defaultAlbedoTexture);
            // Manually set the texture generation to invalid since this is the default texture
            m_defaultAlbedoTexture.generation = INVALID_ID;
        }

        {
            // Normal texture.
            TRACE("Create default normal texture...");
            std::memset(normalPixels, 255, sizeof(u8) * pixelCount * channels);

            // Each pixel
            for (u64 row = 0; row < 16; row++)
            {
                for (u64 col = 0; col < 16; col++)
                {
                    const u64 index    = (row * 16) + col;
                    const u64 indexBpp = index * channels;

                    // Set blue, z-axis by default and alpha
                    normalPixels[indexBpp + 0] = 128;
                    normalPixels[indexBpp + 1] = 128;
                }
            }

            m_defaultNormalTexture = Texture(DEFAULT_NORMAL_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions, channels);
            Renderer.CreateTexture(normalPixels, &m_defaultNormalTexture);
            // Manually set the texture generation to invalid since this is the default texture
            m_defaultNormalTexture.generation = INVALID_ID;
        }

        {
            // Combined texture.
            TRACE("Create default combined(metallic, roughness and ao) texture...");

            for (u32 i = 0; i < totalSize; i += 4)
            {
                combinedPixels[i + 0] = 0;    // R-channel: Default metallic is black (no metallic)
                combinedPixels[i + 1] = 128;  // G-channel: Default roughness is medium gray
                combinedPixels[i + 2] = 255;  // B-channel: Default ao is white
                combinedPixels[i + 3] = 255;  // A-channel is fully opaque
            }

            m_defaultCombinedTexture =
                Texture(DEFAULT_COMBINED_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions, channels);
            Renderer.CreateTexture(combinedPixels, &m_defaultCombinedTexture);
            // Manually set the texture generation to invalid since this is the default texture
            m_defaultCombinedTexture.generation = INVALID_ID;
        }

        {
            // Cube texture.
            TRACE("Create default cube texture...");
            u8 cubeSidePixels[pixelCount * channels];
            std::memset(cubeSidePixels, 255, sizeof(u8) * pixelCount * channels);

            // Each pixel.
            for (u64 row = 0; row < textureDimensions; ++row)
            {
                for (u64 col = 0; col < textureDimensions; ++col)
                {
                    u64 index    = (row * textureDimensions) + col;
                    u64 indexBpp = index * channels;
                    if (row % 2)
                    {
                        if (col % 2)
                        {
                            cubeSidePixels[indexBpp + 1] = 0;
                            cubeSidePixels[indexBpp + 2] = 0;
                        }
                    }
                    else
                    {
                        if (!(col % 2))
                        {
                            cubeSidePixels[indexBpp + 1] = 0;
                            cubeSidePixels[indexBpp + 2] = 0;
                        }
                    }
                }
            }

            u8* pixels    = 0;
            u64 imageSize = 0;
            for (u8 i = 0; i < 6; ++i)
            {
                if (!pixels)
                {
                    m_defaultCubeTexture =
                        Texture(DEFAULT_CUBE_TEXTURE_NAME, TextureTypeCube, textureDimensions, textureDimensions, channels);
                    m_defaultCubeTexture.generation = 0;
                    m_defaultCubeTexture.arraySize  = 6;

                    imageSize = m_defaultCubeTexture.width * m_defaultCubeTexture.height * m_defaultCubeTexture.channelCount;
                    pixels    = Memory.Allocate<u8>(MemoryType::Array, imageSize * 6);
                }

                // Copy to the relevant portion of the array.
                std::memcpy(pixels + imageSize * i, cubeSidePixels, imageSize);
            }

            // Acquire internal texture resources and upload to GPU.
            Renderer.CreateTexture(pixels, &m_defaultCubeTexture);

            if (pixels)
            {
                Memory.Free(pixels);
            }
        }

        {
            // Terrain texture
            u32 layerSize  = sizeof(u8) * 16 * 16 * 4;
            u32 layerCount = 12;

            u8* terrainPixels = Memory.Allocate<u8>(MemoryType::Array, layerSize * layerCount);
            u32 materialSize  = layerSize * 3;
            for (u32 i = 0; i < 4; i++)
            {
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 0), pixels, layerSize);
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 1), normalPixels, layerSize);
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 2), combinedPixels, layerSize);
            }

            m_defaultTerrainTexture.name         = DEFAULT_TERRAIN_TEXTURE_NAME;
            m_defaultTerrainTexture.width        = 16;
            m_defaultTerrainTexture.height       = 16;
            m_defaultTerrainTexture.channelCount = 4;
            m_defaultTerrainTexture.generation   = INVALID_ID;
            m_defaultTerrainTexture.flags        = TextureFlag::None;
            m_defaultTerrainTexture.type         = TextureType2DArray;
            m_defaultTerrainTexture.mipLevels    = 1;
            m_defaultTerrainTexture.arraySize    = layerCount;

            Renderer.CreateTexture(terrainPixels, &m_defaultTerrainTexture);

            // Set id to invalid id since it's a default texture
            m_defaultTerrainTexture.generation = INVALID_ID;

            Memory.Free(terrainPixels);
        }

        return true;
    }

    void TextureSystem::DestroyDefaultTextures()
    {
        DestroyTexture(&m_defaultTexture);
        DestroyTexture(&m_defaultAlbedoTexture);
        DestroyTexture(&m_defaultNormalTexture);
        DestroyTexture(&m_defaultCombinedTexture);
        DestroyTexture(&m_defaultCubeTexture);
        DestroyTexture(&m_defaultTerrainTexture);
    }

    bool TextureSystem::CreateTexture(Texture& texture, TextureType type, u32 width, u32 height, u8 channelCount, u16 arraySize,
                                      const DynamicArray<String>& layerTextureNames, bool isWritable, bool skipLoad)
    {
        texture.type      = type;
        texture.arraySize = arraySize;
        if (isWritable)
        {
            texture.flags |= TextureFlag::IsWritable;
        }

        // Create our new texture
        if (skipLoad)
        {
            texture.width        = width;
            texture.height       = height;
            texture.channelCount = channelCount;
            if (isWritable)
            {
                // Writable textures only have 1 mip level
                texture.mipLevels = 1;
                Renderer.CreateWritableTexture(&texture);
            }
            else
            {
                Renderer.CreateTexture(nullptr, &texture);
            }

            return true;
        }

        // We also need to load our texture
        switch (texture.type)
        {
            case TextureTypeCube:
            {
                CString<TEXTURE_NAME_MAX_LENGTH> textureNames[6];

                // +X,-X,+Y,-Y,+Z,-Z in _cubemap_ space, which is LH y-down
                textureNames[0].FromFormat("{}_r", texture.name);  // Right texture
                textureNames[1].FromFormat("{}_l", texture.name);  // Left texture
                textureNames[2].FromFormat("{}_u", texture.name);  // Up texture
                textureNames[3].FromFormat("{}_d", texture.name);  // Down texture
                textureNames[4].FromFormat("{}_f", texture.name);  // Front texture
                textureNames[5].FromFormat("{}_b", texture.name);  // Back texture

                if (!LoadCubeTextures(textureNames, texture))
                {
                    ERROR_LOG("Failed to load cube texture: '{}'.", texture.name);
                    return false;
                }
            }
            break;
            case TextureType2D:
            case TextureType2DArray:
            {
                if (!LoadTexture(texture, layerTextureNames))
                {
                    ERROR_LOG("Failed to load texture: '{}'.", texture.name);
                    return false;
                }
            }
            break;
            default:
                ERROR_LOG("Unsupported texture type: '{}'.", ToString(texture.type));
                return false;
        }

        return true;
    }

    void TextureSystem::DestroyTexture(Texture* texture) const
    {
        // Cleanup the backend resources for this texture
        Renderer.DestroyTexture(texture);

        // Zero out the memory for the texture
        texture->name.Destroy();

        // Invalidate the id and generation
        texture->id         = INVALID_ID;
        texture->generation = INVALID_ID;
    }

    bool TextureSystem::LoadTexture(Texture& texture, const DynamicArray<String>& layerNames)
    {
        static u32 LOADING_TEXTURE_ID = 0;

        JobInfo info;

        if (texture.type == TextureType2D)
        {
            LoadingTexture loadingTexture;
            loadingTexture.id                    = LOADING_TEXTURE_ID++;
            loadingTexture.resourceName          = texture.name;
            loadingTexture.outTexture            = &texture;
            loadingTexture.currentGeneration     = texture.generation;
            loadingTexture.tempTexture.arraySize = texture.arraySize;

            u32 loadingTextureIndex = INVALID_ID;
            for (auto i = 0; i < m_loadingTextures.Size(); i++)
            {
                if (m_loadingTextures[i].id == INVALID_ID)
                {
                    m_loadingTextures[i] = loadingTexture;
                    loadingTextureIndex  = i;
                    break;
                }
            }

            if (loadingTextureIndex == INVALID_ID)
            {
                ERROR_LOG("Failed to queue texture for loading since there is no space in the loading texture queue.");
                return false;
            }

            info.entryPoint = [this, loadingTextureIndex]() { return LoadTextureEntryPoint(loadingTextureIndex); };
            info.onSuccess  = [this, loadingTextureIndex]() { LoadTextureSuccess(loadingTextureIndex); };
            info.onFailure  = [this, loadingTextureIndex]() {
                const auto& loadingTexture = m_loadingTextures[loadingTextureIndex];

                ERROR_LOG("Failed to load texture '{}'.", loadingTexture.resourceName);
                CleanupLoadingTexture(loadingTextureIndex);
            };
        }
        else if (texture.type == TextureType2DArray)
        {
            LoadingArrayTexture loadingArrayTexture;
            loadingArrayTexture.layerCount            = texture.arraySize;
            loadingArrayTexture.name                  = texture.name;
            loadingArrayTexture.layerNames            = layerNames;
            loadingArrayTexture.currentGeneration     = texture.generation;
            loadingArrayTexture.outTexture            = &texture;
            loadingArrayTexture.tempTexture.arraySize = texture.arraySize;

            u32 loadingArrayTextureIndex = INVALID_ID;
            for (auto i = 0; i < m_loadingArrayTextures.Size(); i++)
            {
                if (m_loadingArrayTextures[i].id == INVALID_ID)
                {
                    m_loadingArrayTextures[i] = loadingArrayTexture;
                    loadingArrayTextureIndex  = i;
                    break;
                }
            }

            if (loadingArrayTextureIndex == INVALID_ID)
            {
                ERROR_LOG("Failed to queue texture for loading since there is no space in the loading texture queue.");
                return false;
            }

            info.entryPoint = [this, loadingArrayTextureIndex]() { return LoadLayeredTextureEntryPoint(loadingArrayTextureIndex); };
            info.onSuccess  = [this, loadingArrayTextureIndex]() { LoadLayeredTextureSuccess(loadingArrayTextureIndex); };
            info.onFailure  = [this, loadingArrayTextureIndex]() {
                const auto& loadingTexture = m_loadingArrayTextures[loadingArrayTextureIndex];

                ERROR_LOG("Failed to load texture '{}'.", loadingTexture.name);
                CleanupLoadingLayeredTexture(loadingArrayTextureIndex);
            };
        }
        else
        {
            ERROR_LOG("Attempted to load texture of unsupported type: '{}'.", ToString(texture.type));
            return false;
        }

        Jobs.Submit(std::move(info));
        TRACE("Loading job submitted for: '{}'.", name);

        return true;
    }

    bool TextureSystem::LoadCubeTextures(const CString<TEXTURE_NAME_MAX_LENGTH>* textureNames, Texture& texture) const
    {
        constexpr ImageLoadParams params{ false };

        u8* pixels    = nullptr;
        u64 imageSize = 0;

        for (auto i = 0; i < 6; i++)
        {
            const auto& textureName = textureNames[i];

            Image res{};
            if (!Resources.Load(textureName.Data(), res, params))
            {
                ERROR_LOG("Failed to load image resource for texture '{}'.", textureName);
                return false;
            }

            if (!res.pixels)
            {
                ERROR_LOG("Failed to load image data for texture '{}'.", textureName);
                return false;
            }

            if (!pixels)
            {
                texture.width        = res.width;
                texture.height       = res.height;
                texture.channelCount = res.channelCount;
                texture.flags        = 0;
                texture.generation   = 0;
                texture.mipLevels    = 1;

                imageSize = static_cast<u64>(texture.width) * texture.height * texture.channelCount;
                pixels    = Memory.Allocate<u8>(MemoryType::Array, imageSize * 6);  // 6 textures one for every side of the cube
            }
            else
            {
                if (texture.width != res.width || texture.height != res.height || texture.channelCount != res.channelCount)
                {
                    ERROR_LOG("Failed to load. All textures must be the same resolution and bit depth.");
                    Memory.Free(pixels);
                    pixels = nullptr;
                    return false;
                }
            }

            // Copy over the pixels to the correct location in the array
            std::memcpy(pixels + (imageSize * i), res.pixels, imageSize);
            // Cleanup our resource
            Resources.Unload(res);
        }

        // Acquire internal texture resources and upload to the GPU
        Renderer.CreateTexture(pixels, &texture);

        Memory.Free(pixels);
        pixels = nullptr;

        return true;
    }

    bool TextureSystem::ProcessTextureReference(const char* name, i8 referenceDiff, bool autoRelease, u32& outTextureId,
                                                bool& outNeedsCreation)
    {
        outTextureId     = INVALID_ID;
        outNeedsCreation = false;

        if (!m_registeredTextures.Has(name))
        {
            // We have no reference to this texture yet
            if (referenceDiff < 0)
            {
                WARN_LOG("Tried to release a non-existant texture.");
                return false;
            }

            m_registeredTextures.Set(name, TextureReference(autoRelease));
        }

        // Get our reference to the texture
        auto& ref = m_registeredTextures.Get(name);

        // Increment/Decrement our reference count
        ref.referenceCount += referenceDiff;

        String nameCopy = name;

        // If decrementing, this means we want to release
        if (referenceDiff < 0)
        {
            // If reference count is 0 and we want to auto release, we destroy the texture
            if (ref.referenceCount == 0 && ref.autoRelease)
            {
                // Destroy the texture
                DestroyTexture(&ref.texture);
                // Delete the reference
                m_registeredTextures.Delete(nameCopy);
                TRACE("Released texture '{}'. Texture unloaded because refCount = 0 and autoRelease = true.", nameCopy);
            }
            else
            {
                // Otherwise we simply do nothing
                TRACE("Released texture '{}'. Texture now has refCount = {} (autoRelease = {}).", nameCopy, ref.referenceCount,
                      ref.autoRelease);
            }

            return true;
        }
        else
        {
            // Incrementing. Check if texture is already valid
            if (ref.texture.id == INVALID_ID)
            {
                // Texture is still invalid so we should load it
                ref.texture.id           = m_registeredTextures.GetIndex(name);
                ref.texture.generation   = INVALID_ID;
                ref.texture.internalData = nullptr;
                ref.texture.name         = name;
                outNeedsCreation         = true;
            }
            else
            {
                TRACE("Texture '{}' already exists. RefCount is now {}.", name, ref.referenceCount);
            }

            outTextureId = ref.texture.id;
        }

        return true;
    }

    bool TextureSystem::LoadTextureEntryPoint(u32 loadingTextureIndex)
    {
        constexpr ImageLoadParams resourceParams{ true };

        auto& loadingTexture = m_loadingTextures[loadingTextureIndex];
        const auto result    = Resources.Load(loadingTexture.resourceName.Data(), loadingTexture.imageResource, resourceParams);
        if (result)
        {
            const auto& resourceData = loadingTexture.imageResource;

            // Use our temporary texture to load into
            loadingTexture.tempTexture.width        = resourceData.width;
            loadingTexture.tempTexture.height       = resourceData.height;
            loadingTexture.tempTexture.channelCount = resourceData.channelCount;
            loadingTexture.tempTexture.mipLevels    = resourceData.mipLevels;
            loadingTexture.tempTexture.type         = loadingTexture.outTexture->type;

            loadingTexture.currentGeneration      = loadingTexture.outTexture->generation;
            loadingTexture.outTexture->generation = INVALID_ID;
            loadingTexture.outTexture->mipLevels  = resourceData.mipLevels;

            const u64 totalSize = static_cast<u64>(loadingTexture.tempTexture.width) * loadingTexture.tempTexture.height *
                                  loadingTexture.tempTexture.channelCount;
            // Check for transparency
            bool hasTransparency = false;
            for (u64 i = 0; i < totalSize; i += loadingTexture.tempTexture.channelCount)
            {
                const u8 a = resourceData.pixels[i + 3];  // Get the alpha channel of this pixel
                if (a < 255)
                {
                    hasTransparency = true;
                    break;
                }
            }

            // Take a copy of the name
            loadingTexture.tempTexture.name       = loadingTexture.resourceName.Data();
            loadingTexture.tempTexture.generation = INVALID_ID;
            loadingTexture.tempTexture.flags |= hasTransparency ? TextureFlag::HasTransparency : 0;
        }

        return result;
    }

    bool TextureSystem::LoadLayeredTextureEntryPoint(u32 loadingTextureIndex)
    {
        constexpr ImageLoadParams resourceParams{ true };

        auto& loadingTexture = m_loadingArrayTextures[loadingTextureIndex];

        bool hasTransparency = false;
        u32 layerSize        = 0;

        for (u32 layer = 0; layer < loadingTexture.layerCount; layer++)
        {
            auto& name = loadingTexture.layerNames[layer];

            if (!Resources.Load(name.Data(), loadingTexture.resource, resourceParams))
            {
                ERROR_LOG("Failed to load texture resources for: '{}'.", name);
                CleanupLoadingLayeredTexture(loadingTextureIndex);
                return false;
            }

            if (layer == 0)
            {
                // First layer so let's save off the the width and height since all folowing textures must match
                loadingTexture.tempTexture.generation   = INVALID_ID;
                loadingTexture.tempTexture.width        = loadingTexture.resource.width;
                loadingTexture.tempTexture.height       = loadingTexture.resource.height;
                loadingTexture.tempTexture.channelCount = loadingTexture.resource.channelCount;
                loadingTexture.tempTexture.mipLevels    = loadingTexture.resource.mipLevels;
                loadingTexture.tempTexture.arraySize    = loadingTexture.layerCount;
                loadingTexture.tempTexture.type         = loadingTexture.outTexture->type;
                loadingTexture.tempTexture.id           = loadingTexture.outTexture->id;
                loadingTexture.tempTexture.flags        = loadingTexture.outTexture->flags;

                constexpr u32 layerChannelCount = 4;
                layerSize = sizeof(u8) * loadingTexture.tempTexture.width * loadingTexture.tempTexture.height * layerChannelCount;
                loadingTexture.dataBlockSize = layerSize * loadingTexture.layerCount;
                loadingTexture.dataBlock     = Memory.Allocate<u8>(MemoryType::Array, loadingTexture.dataBlockSize);
            }
            else
            {
                if (loadingTexture.resource.width != loadingTexture.tempTexture.width ||
                    loadingTexture.resource.height != loadingTexture.tempTexture.height)
                {
                    ERROR_LOG("Texture: '{}' dimensions don't match previous texture which is required.", name);
                    CleanupLoadingLayeredTexture(loadingTextureIndex);
                    return false;
                }
            }

            if (!hasTransparency)
            {
                // Check for transparency only if we haven't come across a transparent texture yet
                for (u64 i = 0; i < layerSize; i += loadingTexture.tempTexture.channelCount)
                {
                    const u8 a = loadingTexture.resource.pixels[i + 3];  // Get the alpha channel of this pixel
                    if (a < 255)
                    {
                        hasTransparency = true;
                        break;
                    }
                }
            }

            // Find the location of our current layer in our total texture and copy the pixels over from our resource
            u8* dataLocation = loadingTexture.dataBlock + (layer * layerSize);
            std::memcpy(dataLocation, loadingTexture.resource.pixels, layerSize);

            Resources.Unload(loadingTexture.resource);
        }

        // Ensure we take transparency into account
        loadingTexture.tempTexture.flags |= hasTransparency ? TextureFlag::HasTransparency : 0;
        // Copy the name
        loadingTexture.tempTexture.name  = loadingTexture.name;
        loadingTexture.currentGeneration = loadingTexture.outTexture->generation;

        return true;
    }

    void TextureSystem::LoadTextureSuccess(u32 loadingTextureIndex)
    {
        // TODO: This still handles the GPU upload. This can't be jobified before our renderer supports multiThreading.
        auto& loadingTexture     = m_loadingTextures[loadingTextureIndex];
        const auto& resourceData = loadingTexture.imageResource;

        // Acquire internal texture resources and upload to GPU.
        Renderer.CreateTexture(resourceData.pixels, &loadingTexture.tempTexture);
        // Take a copy of the old texture.
        Texture old = *loadingTexture.outTexture;
        // Assign the temp texture to the pointer
        *loadingTexture.outTexture = loadingTexture.tempTexture;
        // Destroy the old texture
        Renderer.DestroyTexture(&old);

        if (loadingTexture.currentGeneration == INVALID_ID)
        {
            loadingTexture.outTexture->generation = 0;
        }
        else
        {
            loadingTexture.outTexture->generation++;
        }

        TRACE("Successfully loaded texture: '{}'.", loadingTexture.resourceName);
        CleanupLoadingTexture(loadingTextureIndex);
    }

    void TextureSystem::LoadLayeredTextureSuccess(u32 loadingTextureIndex)
    {
        auto& loadingTexture = m_loadingArrayTextures[loadingTextureIndex];

        // Acquire internal texture resources and upload to GPU.
        Renderer.CreateTexture(loadingTexture.dataBlock, &loadingTexture.tempTexture);

        // Take a copy of the old texture.
        Texture old = *loadingTexture.outTexture;
        // Assign the temp texture to the pointer
        *loadingTexture.outTexture = loadingTexture.tempTexture;
        // Destroy the old texture
        Renderer.DestroyTexture(&old);

        if (loadingTexture.currentGeneration == INVALID_ID)
        {
            loadingTexture.outTexture->generation = 0;
        }
        else
        {
            loadingTexture.outTexture->generation++;
        }

        CleanupLoadingLayeredTexture(loadingTextureIndex);
    }

    void TextureSystem::CleanupLoadingTexture(u32 loadingTextureIndex)
    {
        auto& loadingTexture = m_loadingTextures[loadingTextureIndex];

        // Unload our image resource
        Resources.Unload(loadingTexture.imageResource);
        // Invalidate this loading texture since it is now done
        loadingTexture.id = INVALID_ID;
        loadingTexture.resourceName.Destroy();
    }

    void TextureSystem::CleanupLoadingLayeredTexture(u32 loadingTextureIndex)
    {
        auto& loadingTexture = m_loadingArrayTextures[loadingTextureIndex];

        // Unload our image resource
        Resources.Unload(loadingTexture.resource);
        loadingTexture.name.Destroy();

        if (loadingTexture.dataBlock)
        {
            Memory.Free(loadingTexture.dataBlock);
            loadingTexture.dataBlock     = nullptr;
            loadingTexture.dataBlockSize = 0;
        }
    }
}  // namespace C3D
