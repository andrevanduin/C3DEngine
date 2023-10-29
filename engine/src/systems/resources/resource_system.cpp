
#include "resource_system.h"

#include "core/logger.h"

// Default loaders
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/bitmap_font_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/mesh_loader.h"
#include "resources/loaders/shader_loader.h"
#include "resources/loaders/terrain_loader.h"
#include "resources/loaders/text_loader.h"

namespace C3D
{
    ResourceSystem::ResourceSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager)
    {
        m_loaderTypes[ToUnderlying(ResourceType::None)]        = "None";
        m_loaderTypes[ToUnderlying(ResourceType::Text)]        = "Text";
        m_loaderTypes[ToUnderlying(ResourceType::Binary)]      = "Binary";
        m_loaderTypes[ToUnderlying(ResourceType::Image)]       = "Image";
        m_loaderTypes[ToUnderlying(ResourceType::Material)]    = "Material";
        m_loaderTypes[ToUnderlying(ResourceType::Mesh)]        = "StaticMesh";
        m_loaderTypes[ToUnderlying(ResourceType::Shader)]      = "Shader";
        m_loaderTypes[ToUnderlying(ResourceType::BitmapFont)]  = "BitmapFont";
        m_loaderTypes[ToUnderlying(ResourceType::SystemFont)]  = "SystemFont";
        m_loaderTypes[ToUnderlying(ResourceType::SimpleScene)] = "SimpleScene";
        m_loaderTypes[ToUnderlying(ResourceType::Terrain)]     = "Terrain";
        m_loaderTypes[ToUnderlying(ResourceType::Custom)]      = "Custom";
    }

    bool ResourceSystem::OnInit(const ResourceSystemConfig& config)
    {
        INFO_LOG("Started.");

        if (config.maxLoaderCount == 0)
        {
            FATAL_LOG("Failed because config.maxLoaderCount == 0.");
            return false;
        }

        m_config      = config;
        m_initialized = true;

        const auto textLoader       = Memory.New<ResourceLoader<TextResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto binaryLoader     = Memory.New<ResourceLoader<BinaryResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto imageLoader      = Memory.New<ResourceLoader<Image>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto materialLoader   = Memory.New<ResourceLoader<MaterialConfig>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto shaderLoader     = Memory.New<ResourceLoader<ShaderConfig>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto meshLoader       = Memory.New<ResourceLoader<MeshResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto bitmapFontLoader = Memory.New<ResourceLoader<BitmapFontResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto terrainLoader    = Memory.New<ResourceLoader<TerrainConfig>>(MemoryType::ResourceLoader, m_pSystemsManager);

        for (IResourceLoader* loaders[8] = { textLoader, binaryLoader, imageLoader, materialLoader, shaderLoader, meshLoader,
                                             bitmapFontLoader, terrainLoader };
             const auto loader : loaders)
        {
            if (!RegisterLoader(loader))
            {
                FATAL_LOG("Failed for '{}' loader.", m_loaderTypes[ToUnderlying(loader->type)]);
                return false;
            }
        }

        INFO_LOG("Initialized with base path '{}'.", m_config.assetBasePath);
        return true;
    }

    void ResourceSystem::OnShutdown()
    {
        INFO_LOG("Destroying all registered loaders.");
        for (const auto loader : m_registeredLoaders)
        {
            Memory.Delete(MemoryType::ResourceLoader, loader);
        }
        m_registeredLoaders.Destroy();
    }

    bool ResourceSystem::RegisterLoader(IResourceLoader* newLoader)
    {
        if (!m_initialized) return false;

        for (const auto loader : m_registeredLoaders)
        {
            if (loader->type == newLoader->type)
            {
                ERROR_LOG("A loader of type '{}' already exists so the new one will not be registered.",
                          m_loaderTypes[ToUnderlying(newLoader->type)]);
                return false;
            }
            if (loader->customType && !loader->customType.Empty() && loader->customType.IEquals(newLoader->customType))
            {
                ERROR_LOG("A loader of custom type '{}' already exists so the new one will not be registered.", newLoader->customType);
                return false;
            }
        }

        if (m_registeredLoaders.Size() >= m_config.maxLoaderCount)
        {
            ERROR_LOG("Could not find a free slot for the new resource loader. Increase config.maxLoaderCount.");
            return false;
        }

        newLoader->id = static_cast<u32>(m_registeredLoaders.Size());
        m_registeredLoaders.PushBack(newLoader);
        INFO_LOG("{}Loader registered.", m_loaderTypes[ToUnderlying(newLoader->type)]);
        return true;
    }

    const char* ResourceSystem::GetBasePath() const
    {
        if (m_initialized) return m_config.assetBasePath;

        ERROR_LOG("Called before initialization. Returning empty string.");
        return "";
    }
}  // namespace C3D
