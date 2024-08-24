
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "resources/loaders/image_loader.h"
#include "texture.h"

namespace C3D
{
    /** @brief Structure to hold a texture that is currently loading. */
    class LoadingTexture
    {
    public:
        LoadingTexture(const String& name, Texture* outTexture) : m_name(name), m_outTexture(outTexture) {}

        bool Entry();
        void OnSuccess();
        void Cleanup();

    private:
        String m_name;

        Texture m_texture;
        Texture* m_outTexture = nullptr;

        Image m_image;
    };

    /** @brief Structure to hold a layered texture that is currently loading. */
    class LoadingArrayTexture
    {
    public:
        LoadingArrayTexture(const DynamicArray<String>& names, Texture* outTexture) : m_names(names), m_outTexture(outTexture) {}

        bool Entry();
        void OnSuccess();
        void Cleanup();

    private:
        DynamicArray<String> m_names;

        Texture m_texture;
        Texture* m_outTexture = nullptr;

        u64 m_dataBlockSize = 0;
        u8* m_dataBlock     = nullptr;
    };
}  // namespace C3D