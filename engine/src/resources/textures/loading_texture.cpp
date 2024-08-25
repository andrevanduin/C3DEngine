
#include "loading_texture.h"

#include <future>

#include "core/scoped_timer.h"
#include "renderer/renderer_frontend.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    bool LoadingTexture::Entry()
    {
        constexpr ImageLoadParams resourceParams{ true };

        const auto result = Resources.Load(m_name.Data(), m_image, resourceParams);
        if (result)
        {
            // Use our temporary texture to load into
            m_texture.width        = m_image.width;
            m_texture.height       = m_image.height;
            m_texture.channelCount = m_image.channelCount;
            m_texture.mipLevels    = m_image.mipLevels;

            // Copy the name, type, handle, arraySize, generation and flags from our out texture
            m_texture.name       = m_outTexture->name;
            m_texture.type       = m_outTexture->type;
            m_texture.handle     = m_outTexture->handle;
            m_texture.arraySize  = m_outTexture->arraySize;
            m_texture.generation = m_outTexture->generation;
            m_texture.flags      = m_outTexture->flags;

            const u64 totalSize = static_cast<u64>(m_image.width) * m_image.height * m_image.channelCount;
            // Check for transparency
            bool hasTransparency = false;
            for (u64 i = 0; i < totalSize; i += m_image.channelCount)
            {
                const u8 a = m_image.pixels[i + 3];  // Get the alpha channel of this pixel
                if (a < 255)
                {
                    hasTransparency = true;
                    break;
                }
            }

            if (hasTransparency)
            {
                // Ensure we set the transparency flag if required
                m_texture.flags |= TextureFlag::HasTransparency;
            }
            else
            {
                // Ensure we remove the transparency flag if not required
                m_texture.flags &= ~TextureFlag::HasTransparency;
            }
        }

        return result;
    }

    void LoadingTexture::OnSuccess()
    {
        // TODO: This still handles the GPU upload. This can't be jobified before our renderer supports multiThreading.

        // Acquire internal texture resources and upload to GPU.
        Renderer.CreateTexture(m_texture, m_image.pixels);
        // Take a copy of the old texture.
        Texture old = *m_outTexture;
        // Assign the temp texture to the pointer
        *m_outTexture = m_texture;
        // Destroy the old texture
        Renderer.DestroyTexture(old);
        // Increment our out texture's generation
        m_outTexture->generation++;

        INFO_LOG("Successfully loaded texture: '{}'.", m_name);
        Cleanup();
    }

    void LoadingTexture::Cleanup()
    {
        // Unload our image resource
        Resources.Unload(m_image);
        // Destroy the resource name
        m_name.Destroy();
        // Destroy the underlying memory
        Memory.Delete(this);
    }

    struct AsyncResult
    {
        Image image;
        bool success = true;
    };

    AsyncResult LoadLayeredTextureLayer(const char* name)
    {
        AsyncResult result;

        constexpr ImageLoadParams resourceParams{ true };
        result.success = Resources.Load(name, result.image, resourceParams);
        if (!result.success)
        {
            Resources.Unload(result.image);
            ERROR_LOG("Failed to load texture resources for: '{}'.", name);
        }
        return result;
    }

    bool LoadingArrayTexture::Entry()
    {
        auto timer = ScopedTimer("LoadLayeredTexture");

        bool hasTransparency = false;
        u32 layerSize        = 0;
        u32 layerCount       = m_names.Size();

        // Load the resources in parallel
        std::future<AsyncResult> results[12];

        for (u32 i = 0; i < layerCount; ++i)
        {
            results[i] = std::async(LoadLayeredTextureLayer, m_names[i].Data());
        }

        for (u32 layer = 0; layer < layerCount; ++layer)
        {
            auto result = results[layer].get();
            if (!result.success)
            {
                // Returning false here will cause Cleanup() to be called by the JobSystem
                ERROR_LOG("Texture: '{}' failed to load because layer: '{}' failed to load.", m_outTexture->name, m_names[layer]);
                return false;
            }

            if (layer == 0)
            {
                // First layer so let's save off the the width and height since all folowing textures must match
                m_texture.width        = result.image.width;
                m_texture.height       = result.image.height;
                m_texture.channelCount = result.image.channelCount;
                m_texture.mipLevels    = result.image.mipLevels;

                // Copy the name, type, handle, arraySize, generation and flags from our out texture
                m_texture.name       = m_outTexture->name;
                m_texture.type       = m_outTexture->type;
                m_texture.handle     = m_outTexture->handle;
                m_texture.arraySize  = layerCount;
                m_texture.generation = m_outTexture->generation;
                m_texture.flags      = m_outTexture->flags;

                layerSize       = sizeof(u8) * result.image.width * result.image.height * result.image.channelCount;
                m_dataBlockSize = layerSize * layerCount;
                m_dataBlock     = Memory.Allocate<u8>(MemoryType::Array, m_dataBlockSize);
            }
            else
            {
                if (result.image.width != m_texture.width || result.image.height != m_texture.height)
                {
                    ERROR_LOG(
                        "Texture: '{}' failed to load because the dimensions of layer: '{}' don't match previous texture which is "
                        "required.",
                        m_outTexture->name, m_names[layer]);
                    // Returning false here will cause Cleanup() to be called by the JobSystem
                    return false;
                }
            }

            if (!hasTransparency)
            {
                // Check for transparency only if we haven't come across a transparent texture yet
                for (u64 i = 0; i < layerSize; i += m_texture.channelCount)
                {
                    const u8 a = result.image.pixels[i + 3];  // Get the alpha channel of this pixel
                    if (a < 255)
                    {
                        hasTransparency = true;
                        break;
                    }
                }
            }

            // Find the location of our current layer in our total texture and copy the pixels over from our resource
            u8* dataLocation = m_dataBlock + (layer * layerSize);
            std::memcpy(dataLocation, result.image.pixels, layerSize);

            Resources.Unload(result.image);
        }

        if (hasTransparency)
        {
            // Ensure we set the transparency flag if required
            m_texture.flags |= TextureFlag::HasTransparency;
        }
        else
        {
            // Ensure we remove the transparency flag if not required
            m_texture.flags &= ~TextureFlag::HasTransparency;
        }

        return true;
    }

    void LoadingArrayTexture::OnSuccess()
    {
        // Acquire internal texture resources and upload to GPU.
        Renderer.CreateTexture(m_texture, m_dataBlock);
        // Take a copy of the old texture.
        Texture old = *m_outTexture;
        // Assign the temp texture to the pointer
        *m_outTexture = m_texture;
        // Destroy the old texture
        Renderer.DestroyTexture(old);
        // Increment our out texture's generation
        m_outTexture->generation++;

        INFO_LOG("Successfully loaded texture: '{}'.", m_outTexture->name);
        Cleanup();
    }

    void LoadingArrayTexture::Cleanup()
    {
        // Destroy our layer names and temp texture name
        m_names.Destroy();
        m_texture.name.Destroy();

        if (m_dataBlock && m_dataBlockSize > 0)
        {
            // We have some data which we should free
            Memory.Free(m_dataBlock);
            m_dataBlock     = nullptr;
            m_dataBlockSize = 0;
        }

        // Finally we free this memory
        Memory.Delete(this);
    }
}  // namespace C3D