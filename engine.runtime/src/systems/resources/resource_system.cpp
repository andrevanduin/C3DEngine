
#include "resource_system.h"

#include "logger/logger.h"

// Default loaders
#include "resources/managers/audio_manager.h"
#include "resources/managers/binary_manager.h"
#include "resources/managers/bitmap_font_manager.h"
#include "resources/managers/image_manager.h"
#include "resources/managers/material_manager.h"
#include "resources/managers/mesh_manager.h"
#include "resources/managers/scene_manager.h"
#include "resources/managers/shader_manager.h"
#include "resources/managers/terrain_manager.h"
#include "resources/managers/text_manager.h"

namespace C3D
{
    ResourceSystem::ResourceSystem()
    {
        m_resourceManagerTypes[ToUnderlying(ResourceType::None)]       = "None";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Text)]       = "Text";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Binary)]     = "Binary";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Image)]      = "Image";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Material)]   = "Material";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Mesh)]       = "StaticMesh";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Shader)]     = "Shader";
        m_resourceManagerTypes[ToUnderlying(ResourceType::BitmapFont)] = "BitmapFont";
        m_resourceManagerTypes[ToUnderlying(ResourceType::SystemFont)] = "SystemFont";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Scene)]      = "Scene";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Terrain)]    = "Terrain";
        m_resourceManagerTypes[ToUnderlying(ResourceType::AudioFile)]  = "Audio";
        m_resourceManagerTypes[ToUnderlying(ResourceType::Custom)]     = "Custom";
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

        const auto textLoader       = Memory.New<ResourceManager<TextResource>>(MemoryType::ResourceLoader);
        const auto binaryLoader     = Memory.New<ResourceManager<BinaryResource>>(MemoryType::ResourceLoader);
        const auto imageLoader      = Memory.New<ResourceManager<Image>>(MemoryType::ResourceLoader);
        const auto materialLoader   = Memory.New<ResourceManager<MaterialConfig>>(MemoryType::ResourceLoader);
        const auto shaderLoader     = Memory.New<ResourceManager<ShaderConfig>>(MemoryType::ResourceLoader);
        const auto meshLoader       = Memory.New<ResourceManager<MeshResource>>(MemoryType::ResourceLoader);
        const auto bitmapFontLoader = Memory.New<ResourceManager<BitmapFontResource>>(MemoryType::ResourceLoader);
        const auto terrainLoader    = Memory.New<ResourceManager<TerrainConfig>>(MemoryType::ResourceLoader);
        const auto audioLoader      = Memory.New<ResourceManager<AudioFile>>(MemoryType::ResourceLoader);
        const auto sceneLoader      = Memory.New<ResourceManager<SceneConfig>>(MemoryType::ResourceLoader);

        IResourceManager* managers[10] = { textLoader, binaryLoader,     imageLoader,   materialLoader, shaderLoader,
                                           meshLoader, bitmapFontLoader, terrainLoader, audioLoader,    sceneLoader };

        m_registeredManagers.Resize(16);

        for (const auto manager : managers)
        {
            if (!RegisterManager(manager))
            {
                FATAL_LOG("Failed for '{}' manager.", m_resourceManagerTypes[ToUnderlying(manager->type)]);
                return false;
            }
        }

        INFO_LOG("Initialized with base path '{}'.", m_config.assetBasePath);
        return true;
    }

    void ResourceSystem::OnShutdown()
    {
        INFO_LOG("Destroying all registered loaders.");
        for (const auto manager : m_registeredManagers)
        {
            if (manager)
            {
                Memory.Delete(manager);
            }
        }
        m_registeredManagers.Destroy();
    }

    bool ResourceSystem::RegisterManager(IResourceManager* newManager)
    {
        if (!m_initialized) return false;

        if (newManager->id == INVALID_ID_U16)
        {
            ERROR_LOG("Manager has an invalid id.");
            return false;
        }

        if (m_registeredManagers[newManager->id])
        {
            ERROR_LOG("Manager at index: {} already exists.", newManager->id);
            return false;
        }

        m_registeredManagers[newManager->id] = newManager;

        INFO_LOG("{}Manager registered.", m_resourceManagerTypes[ToUnderlying(newManager->type)]);
        return true;
    }

    const char* ResourceSystem::GetBasePath() const
    {
        if (m_initialized) return m_config.assetBasePath;

        ERROR_LOG("Called before initialization. Returning empty string.");
        return "";
    }
}  // namespace C3D
