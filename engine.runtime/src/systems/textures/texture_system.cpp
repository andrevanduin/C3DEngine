
#include "texture_system.h"

#include <future>

#include "engine.h"
#include "jobs/job.h"
#include "logger/logger.h"
#include "renderer/renderer_frontend.h"
#include "resources/managers/image_manager.h"
#include "resources/textures/loading_texture.h"
#include "string/string_utils.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"
#include "time/scoped_timer.h"

namespace C3D
{
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
        m_textures.Reserve(config.maxTextureCount);
        m_nameToTextureIndexMap.Create();

        m_initialized = true;
        return true;
    }

    void TextureSystem::OnShutdown()
    {
        INFO_LOG("Destroying all loaded textures.");
        for (auto& ref : m_textures)
        {
            // Destroy all non-wrapped textures
            if (!ref.texture.IsWrapped())
            {
                Renderer.DestroyTexture(ref.texture);
            }
        }

        // Free the memory that was storing all the textures
        m_textures.Destroy();
        // Destroy the map that mapped names to Texture indices
        m_nameToTextureIndexMap.Destroy();
    }

    TextureHandle TextureSystem::Acquire(const String& name, bool autoRelease)
    {
        if (TextureReferenceExists(name))
        {
            // We already have this texture
            auto& ref = GetTextureReference(name);
            // Increment our reference count
            ref.referenceCount++;
            // And return the Texture id which is used as handle
            return ref.texture.handle;
        }

        // The texture does not exist yet
        auto& ref     = CreateTextureReference(name, autoRelease);
        auto& texture = ref.texture;

        // Set the specific arguments for this type of texture
        texture.type      = TextureType2D;
        texture.arraySize = 1;

        LoadTexture(texture);

        return texture.handle;
    }

    TextureHandle TextureSystem::AcquireArray(const String& name, u32 layerCount, const DynamicArray<String>& layerTextureNames,
                                              bool autoRelease)
    {
        if (layerCount < 1)
        {
            ERROR_LOG("A texture must contain at least 1 layer.");
            return INVALID_ID;
        }

        if (TextureReferenceExists(name))
        {
            // We already have this texture
            auto& ref = GetTextureReference(name);
            // Increment our reference count
            ref.referenceCount++;
            // And return the Texture id which is used as handle
            return ref.texture.handle;
        }

        // The texture does not exist yet
        auto& ref     = CreateTextureReference(name, autoRelease);
        auto& texture = ref.texture;

        // Set the specific arguments for this type of texture
        texture.type      = TextureType2DArray;
        texture.arraySize = layerCount;

        LoadArrayTexture(texture, layerTextureNames);

        return texture.handle;
    }

    TextureHandle TextureSystem::AcquireCube(const String& name, bool autoRelease)
    {
        if (TextureReferenceExists(name))
        {
            // We already have this texture
            auto& ref = GetTextureReference(name);
            // Increment our reference count
            ref.referenceCount++;
            // And return the Texture id which is used as handle
            return ref.texture.handle;
        }

        // The texture does not exist yet
        auto& ref     = CreateTextureReference(name, autoRelease);
        auto& texture = ref.texture;

        // Set the specific arguments for this type of texture
        texture.type      = TextureTypeCube;
        texture.arraySize = 6;

        CString<TEXTURE_NAME_MAX_LENGTH> textureNames[6];

        // +X,-X,+Y,-Y,+Z,-Z in _cubemap_ space, which is LH y-down
        textureNames[0].FromFormat("{}_r", texture.name);  // Right texture
        textureNames[1].FromFormat("{}_l", texture.name);  // Left texture
        textureNames[2].FromFormat("{}_u", texture.name);  // Up texture
        textureNames[3].FromFormat("{}_d", texture.name);  // Down texture
        textureNames[4].FromFormat("{}_f", texture.name);  // Front texture
        textureNames[5].FromFormat("{}_b", texture.name);  // Back texture

        if (!LoadCubeTexture(textureNames, texture))
        {
            ERROR_LOG("Failed to load cube texture: '{}'.", texture.name);
            DeleteTextureReference(name);
            return INVALID_ID;
        }

        return texture.handle;
    }

    TextureHandle TextureSystem::AcquireWritable(const String& name, u32 width, u32 height, u8 channelCount, TextureFlagBits flags)
    {
        return CreateArrayWritable(name, TextureType2D, width, height, channelCount, 1, flags);
    }

    TextureHandle TextureSystem::AcquireArrayWritable(const String& name, u32 width, u32 height, u8 channelCount, u16 arraySize,
                                                      TextureFlagBits flags)
    {
        return CreateArrayWritable(name, TextureType2DArray, width, height, channelCount, arraySize, flags);
    }

    void TextureSystem::Release(const String& name)
    {
        if (!m_nameToTextureIndexMap.Has(name))
        {
            WARN_LOG("Tried to release a non-existant texture: '{}'.", name);
            return;
        }

        // Get our index and reference
        auto index = m_nameToTextureIndexMap.Get(name);
        auto& ref  = m_textures[index];
        // Decrement the reference count
        ref.referenceCount--;

        if (ref.autoRelease && ref.referenceCount == 0)
        {
            INFO_LOG("Texture: '{}' was released because autoRelease == true and referenceCount == 0.", name);

            // Delete the reference
            m_nameToTextureIndexMap.Delete(name);
            // Destroy the texture
            DestroyTexture(ref.texture);
            // And mark this slot as unoccupied
            m_textures[index].texture.handle = INVALID_ID;
        }
    }

    void TextureSystem::Release(TextureHandle handle)
    {
#ifdef _DEBUG
        if (handle >= m_textures.Size())
        {
            FATAL_LOG("Tried calling Release() with an invalid handle.");
        }
#endif

        // Get the reference to our texture by the handle
        auto& ref = m_textures[handle];
        // Decrement the reference count
        ref.referenceCount--;

        if (ref.autoRelease && ref.referenceCount == 0)
        {
            // Delete the reference
            m_nameToTextureIndexMap.Delete(ref.texture.name);
            // Destroy the texture
            DestroyTexture(ref.texture);
            // And mark this slot as unoccupied
            m_textures[handle].texture.handle = INVALID_ID;
        }
    }

    Texture& TextureSystem::WrapInternal(const char* name, u32 width, u32 height, u8 channelCount, void* internalData)
    {
        if (TextureReferenceExists(name))
        {
            // This reference already exists
            auto& ref = GetTextureReference(name);
            // Increment the reference count
            ref.referenceCount++;
            // Set the potentially changed properties
            ref.texture.width        = width;
            ref.texture.height       = height;
            ref.texture.channelCount = channelCount;
            ref.texture.internalData = internalData;
            // Return the texture
            return ref.texture;
        }

        // We don't autoRelease wrapped textures since we don't manage them ourselves
        auto& ref = CreateTextureReference(name, false);

        // Set all the properties
        ref.texture.type         = TextureType2D;
        ref.texture.width        = width;
        ref.texture.height       = height;
        ref.texture.channelCount = channelCount;
        ref.texture.internalData = internalData;
        ref.texture.flags        = IsWrapped;
        ref.texture.internalData = internalData;
        // Return the texture
        return ref.texture;
    }

    void TextureSystem::ReleaseInternal(Texture& texture)
    {
        if (!m_textures.Empty())
        {
            auto& ref = m_textures[texture.handle];
            if (ref.referenceCount == 0)
            {
                WARN_LOG("Tried to release texture with reference count == 0.");
            }
            // We just reduce the reference count since internal (wrapped) textures are managed elsewhere
            ref.referenceCount--;
        }
    }

    bool TextureSystem::Resize(Texture& texture, const u32 width, const u32 height, const bool regenerateInternalData) const
    {
        if (!texture.IsWritable())
        {
            WARN_LOG("Should not be called on textures that are not writable.");
            return false;
        }

        // Set the new width and height
        texture.width  = width;
        texture.height = height;

        // If the texture is owned by the texture system (not wrapped) and it's requested to regenerate the internal data we resize
        if (!texture.IsWrapped() && regenerateInternalData)
        {
            Renderer.ResizeTexture(texture, width, height);
            return false;
        }

        // Increase the texture's generation since we have made changes
        texture.generation++;
        return true;
    }

    void TextureSystem::WriteData(Texture& texture, const u32 offset, const u32 size, const u8* data) const
    {
        Renderer.WriteDataToTexture(texture, offset, size, data);
    }

    TextureHandle TextureSystem::GetDefault()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultTexture;
    }

    TextureHandle TextureSystem::GetDefaultDiffuse()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultAlbedoTexture;
    }

    TextureHandle TextureSystem::GetDefaultAlbedo()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultAlbedoTexture;
    }

    TextureHandle TextureSystem::GetDefaultNormal()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultNormalTexture;
    }

    TextureHandle TextureSystem::GetDefaultCombined()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultCombinedTexture;
    }

    TextureHandle TextureSystem::GetDefaultCube()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultCubeTexture;
    }

    TextureHandle TextureSystem::GetDefaultTerrain()
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning invalid handle.");
            return INVALID_ID;
        }
        return m_defaultTerrainTexture;
    }

    bool TextureSystem::IsDefault(TextureHandle handle) const
    {
        if (!m_initialized)
        {
            ERROR_LOG("Was called before initialization. Returning false.");
            return false;
        }
        return (handle == m_defaultTexture) || (handle == m_defaultAlbedoTexture) || (handle == m_defaultNormalTexture) ||
               (handle == m_defaultCombinedTexture) || (handle == m_defaultCubeTexture) || (handle == m_defaultTerrainTexture);
    }

    const Texture& TextureSystem::Get(TextureHandle handle) const
    {
#ifdef _DEBUG
        if (handle == INVALID_ID || handle >= m_textures.Size())
        {
            FATAL_LOG("Tried to get a non-existant texture: '{}'", handle);
        }
#endif
        return m_textures[handle].texture;
    }

    const String& TextureSystem::GetName(TextureHandle handle) const
    {
#ifdef _DEBUG
        if (handle == INVALID_ID || handle >= m_textures.Size())
        {
            FATAL_LOG("Tried to get the name of a non-existant texture: '{}'.", handle);
        }
#endif
        return m_textures[handle].texture.name;
    }

    bool TextureSystem::HasTransparency(TextureHandle handle) const
    {
#ifdef _DEBUG
        if (handle == INVALID_ID || handle >= m_textures.Size())
        {
            FATAL_LOG("Tried to get the transparency of a a non-existant texture: '{}'.", handle);
        }
#endif
        return m_textures[handle].texture.HasTransparency();
    }

    TextureHandle TextureSystem::CreateDefaultTexture(const String& name, TextureType type, u32 width, u32 height, u8 channelCount,
                                                      u8* pixels, u16 arraySize)
    {
        // Default textures should only be released at engine shutdown
        auto& ref = CreateTextureReference(name, false);
        // Initialize our texture
        ref.texture.type         = type;
        ref.texture.width        = width;
        ref.texture.height       = height;
        ref.texture.channelCount = channelCount;
        ref.texture.arraySize    = arraySize;
        // Let the renderer backend handle creating our texture's internals
        Renderer.CreateTexture(ref.texture, pixels);
        // Finally set our generation to invalid to indicate that this is a DEFAULT texture
        ref.texture.generation = INVALID_ID;
        // Return a handle to the texture
        return ref.texture.handle;
    }

    TextureHandle TextureSystem::CreateArrayWritable(const String& name, TextureType type, u32 width, u32 height, u8 channelCount,
                                                     u16 arraySize, TextureFlagBits flags)
    {
        if (TextureReferenceExists(name))
        {
            // We already have this texture
            auto& ref = GetTextureReference(name);
            // Increment our reference count
            ref.referenceCount++;
            // And return the Texture id which is used as handle
            return ref.texture.handle;
        }

        // The texture does not exist yet (we hardcode autoRelease since writable textures should be manually released)
        auto& ref     = CreateTextureReference(name, false);
        auto& texture = ref.texture;

        // Set the specific arguments required for this type of texture
        texture.type         = type;
        texture.width        = width;
        texture.height       = height;
        texture.arraySize    = arraySize;
        texture.channelCount = channelCount;
        // Writable textures should have just 1 mip level
        texture.mipLevels = 1;
        // Set the writable flag
        texture.flags |= TextureFlag::IsWritable;
        // Set the user provided flags
        texture.flags |= flags;

        Renderer.CreateWritableTexture(texture);

        return texture.handle;
    }

    void TextureSystem::CreateDefaultTextures()
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

            m_defaultTexture =
                CreateDefaultTexture(DEFAULT_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions, channels, pixels);
        }

        {
            // Albedo texture
            TRACE("Create default albedo texture...");
            // Default albedo texture is all white
            std::memset(albedoPixels, 255, sizeof(u8) * pixelCount * channels);  // Default albedo map is all white

            m_defaultAlbedoTexture = CreateDefaultTexture(DEFAULT_ALBEDO_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions,
                                                          channels, albedoPixels);
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

            m_defaultNormalTexture = CreateDefaultTexture(DEFAULT_NORMAL_TEXTURE_NAME, TextureType2D, textureDimensions, textureDimensions,
                                                          channels, normalPixels);
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

            m_defaultCombinedTexture = CreateDefaultTexture(DEFAULT_COMBINED_TEXTURE_NAME, TextureType2D, textureDimensions,
                                                            textureDimensions, channels, combinedPixels);
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

            u64 imageSize = textureDimensions * textureDimensions * channels;
            u8* pixels    = Memory.Allocate<u8>(MemoryType::Array, imageSize * 6);

            for (u8 i = 0; i < 6; ++i)
            {
                // Copy to the relevant portion of the array.
                std::memcpy(pixels + imageSize * i, cubeSidePixels, imageSize);
            }

            m_defaultCubeTexture =
                CreateDefaultTexture(DEFAULT_CUBE_TEXTURE_NAME, TextureTypeCube, textureDimensions, textureDimensions, channels, pixels, 6);

            if (pixels)
            {
                Memory.Free(pixels);
            }
        }

        {
            // Terrain texture
            u32 layerSize  = sizeof(u8) * textureDimensions * textureDimensions * channels;
            u32 layerCount = 12;

            u8* terrainPixels = Memory.Allocate<u8>(MemoryType::Array, layerSize * layerCount);
            u32 materialSize  = layerSize * 3;
            for (u32 i = 0; i < 4; i++)
            {
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 0), pixels, layerSize);
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 1), normalPixels, layerSize);
                std::memcpy(terrainPixels + (materialSize * i) + (layerSize * 2), combinedPixels, layerSize);
            }

            m_defaultTerrainTexture = CreateDefaultTexture(DEFAULT_TERRAIN_TEXTURE_NAME, TextureType2DArray, textureDimensions,
                                                           textureDimensions, channels, terrainPixels, layerCount);

            Memory.Free(terrainPixels);
        }
    }

    bool TextureSystem::TextureReferenceExists(const String& name) const { return m_nameToTextureIndexMap.Has(name); }

    TextureReference& TextureSystem::CreateTextureReference(const String& name, bool autoRelease)
    {
#ifdef _DEBUG
        if (m_nameToTextureIndexMap.Has(name))
        {
            FATAL_LOG("Texture name: '{}' already exists.", name);
        }
#endif

        // Creata a new TextureReference
        u32 index = INVALID_ID_U32;
        for (u32 i = 0; i < m_textures.Size(); ++i)
        {
            auto& current = m_textures[i];

            if (!current.texture.handle == INVALID_ID)
            {
                // We have found an empty slot
                current = TextureReference(autoRelease);
                index   = i;
                break;
            }
        }

        if (index == INVALID_ID)
        {
            // We did not find a spot so let's append the TextureReference
            index = m_textures.Size();
            m_textures.EmplaceBack(autoRelease);
        }

        m_nameToTextureIndexMap.Set(name, index);

        // Get our reference
        auto& ref = m_textures[index];
        // Set the id
        ref.texture.handle = index;
        // Set the name
        ref.texture.name = name;
        // and finally return the reference
        return ref;
    }

    TextureReference& TextureSystem::GetTextureReference(const String& name)
    {
        auto index = m_nameToTextureIndexMap.Get(name);
        return m_textures[index];
    }

    void TextureSystem::DeleteTextureReference(const String& name)
    {
#ifdef _DEBUG
        if (!m_nameToTextureIndexMap.Has(name))
        {
            ERROR_LOG("Tried to delete a non-existant Texture Reference");
            return;
        }
#endif
        auto index = m_nameToTextureIndexMap.Get(name);
        // Mark the texture as invalid
        m_textures[index].texture.handle = INVALID_ID;
        // Delete the name from the map
        m_nameToTextureIndexMap.Delete(name);
    }

    void TextureSystem::DestroyTexture(Texture& texture) const
    {
        // Cleanup the backend resources for this texture
        Renderer.DestroyTexture(texture);

        // Zero out the memory for the texture
        texture.name.Destroy();

        // Invalidate the id and generation
        texture.handle     = INVALID_ID;
        texture.generation = INVALID_ID;
    }

    void TextureSystem::LoadTexture(Texture& texture)
    {
        auto load = Memory.New<LoadingTexture>(MemoryType::Job, texture.name, &texture);
        Jobs.Submit([load]() { return load->Entry(); }, [load]() { load->OnSuccess(); }, [load]() { load->Cleanup(); });
    }

    void TextureSystem::LoadArrayTexture(Texture& texture, const DynamicArray<String>& layerNames)
    {
        auto load = Memory.New<LoadingArrayTexture>(MemoryType::Job, layerNames, &texture);
        Jobs.Submit([load]() { return load->Entry(); }, [load]() { load->OnSuccess(); }, [load]() { load->Cleanup(); });
    }

    bool TextureSystem::LoadCubeTexture(const CString<TEXTURE_NAME_MAX_LENGTH>* textureNames, Texture& texture) const
    {
        constexpr ImageLoadParams params{ false };

        u8* pixels    = nullptr;
        u64 imageSize = 0;

        for (auto i = 0; i < 6; i++)
        {
            const auto& textureName = textureNames[i];

            Image res{};
            if (!Resources.Read(textureName.Data(), res, params))
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
            Resources.Cleanup(res);
        }

        // Acquire internal texture resources and upload to the GPU
        Renderer.CreateTexture(texture, pixels);

        Memory.Free(pixels);
        pixels = nullptr;

        return true;
    }
}  // namespace C3D
