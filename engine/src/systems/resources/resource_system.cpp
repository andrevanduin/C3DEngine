
#include "resource_system.h"

#include "core/logger.h"

// Default loaders
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/bitmap_font_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/mesh_loader.h"
#include "resources/loaders/shader_loader.h"
#include "resources/loaders/text_loader.h"

namespace C3D
{
    ResourceSystem::ResourceSystem(const SystemManager* pSystemsManager)
        : SystemWithConfig(pSystemsManager, "RESOURCE_SYSTEM"), m_loaderTypes{}
    {
        m_loaderTypes[ToUnderlying(ResourceType::None)] = "None";
        m_loaderTypes[ToUnderlying(ResourceType::Text)] = "Text";
        m_loaderTypes[ToUnderlying(ResourceType::Binary)] = "Binary";
        m_loaderTypes[ToUnderlying(ResourceType::Image)] = "Image";
        m_loaderTypes[ToUnderlying(ResourceType::Material)] = "Material";
        m_loaderTypes[ToUnderlying(ResourceType::Mesh)] = "StaticMesh";
        m_loaderTypes[ToUnderlying(ResourceType::Shader)] = "Shader";
        m_loaderTypes[ToUnderlying(ResourceType::BitmapFont)] = "BitmapFont";
        m_loaderTypes[ToUnderlying(ResourceType::SystemFont)] = "SystemFont";
        m_loaderTypes[ToUnderlying(ResourceType::Custom)] = "Custom";
    }

    bool ResourceSystem::Init(const ResourceSystemConfig& config)
    {
        if (config.maxLoaderCount == 0)
        {
            m_logger.Fatal("Init() failed because config.maxLoaderCount == 0");
            return false;
        }

        m_config = config;
        m_initialized = true;

        const auto textLoader = Memory.New<ResourceLoader<TextResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto binaryLoader =
            Memory.New<ResourceLoader<BinaryResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto imageLoader =
            Memory.New<ResourceLoader<ImageResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto materialLoader =
            Memory.New<ResourceLoader<MaterialResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto shaderLoader =
            Memory.New<ResourceLoader<ShaderResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto meshLoader = Memory.New<ResourceLoader<MeshResource>>(MemoryType::ResourceLoader, m_pSystemsManager);
        const auto bitmapFontLoader =
            Memory.New<ResourceLoader<BitmapFontResource>>(MemoryType::ResourceLoader, m_pSystemsManager);

        for (IResourceLoader* loaders[7] = {textLoader, binaryLoader, imageLoader, materialLoader, shaderLoader,
                                            meshLoader, bitmapFontLoader};
             const auto loader : loaders)
        {
            if (!RegisterLoader(loader))
            {
                m_logger.Fatal("RegisterLoader() failed for {} loader", m_loaderTypes[ToUnderlying(loader->type)]);
                return false;
            }
        }

        m_logger.Info("Initialized with base path '{}'", m_config.assetBasePath);
        return true;
    }

    void ResourceSystem::Shutdown()
    {
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
                m_logger.Error(
                    "RegisterLoader() - A loader of type '{}' already exists so the new one will not be registered",
                    m_loaderTypes[ToUnderlying(newLoader->type)]);
                return false;
            }
            if (loader->customType && !loader->customType.Empty() && loader->customType.IEquals(newLoader->customType))
            {
                m_logger.Error(
                    "RegisterLoader() - A loader of custom type '{}' already exists so the new one will not be "
                    "registered",
                    newLoader->customType);
                return false;
            }
        }

        if (m_registeredLoaders.Size() >= m_config.maxLoaderCount)
        {
            m_logger.Error(
                "RegisterLoader() - Could not find a free slot for the new resource loader. Increase "
                "config.maxLoaderCount");
            return false;
        }

        newLoader->id = static_cast<u32>(m_registeredLoaders.Size());
        m_registeredLoaders.PushBack(newLoader);
        m_logger.Trace("{}Loader registered", m_loaderTypes[ToUnderlying(newLoader->type)]);
        return true;
    }

    const char* ResourceSystem::GetBasePath() const
    {
        if (m_initialized) return m_config.assetBasePath;

        m_logger.Error("GetBasePath() called before initialization. Returning empty string");
        return "";
    }
}  // namespace C3D
