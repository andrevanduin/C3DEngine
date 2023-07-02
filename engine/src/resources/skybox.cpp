
#include "skybox.h"

#include "renderer/renderer_frontend.h"
#include "systems/geometry/geometry_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    Skybox::Skybox() : m_logger("SKYBOX") {}

    bool Skybox::Create(const SystemManager* pSystemsManager, const SkyboxConfig& config)
    {
        m_pSystemsManager = pSystemsManager;
        m_config = config;
        return true;
    }

    bool Skybox::Initialize()
    {
        cubeMap = TextureMap(TextureUse::CubeMap, TextureFilter::ModeLinear, TextureRepeat::ClampToEdge);

        // Generate a config for some cube geometry
        // TODO: Make these settings more configurable
        m_skyboxGeometryConfig =
            Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, m_config.cubeMapName, "");
        // Clear out the material name
        m_skyboxGeometryConfig.materialName[0] = '\0';
        // Invalidate our id so it's clear that this skybox still needs to be loaded
        instanceId = INVALID_ID;

        return true;
    }

    bool Skybox::Load()
    {
        // Acquire resources for our cube map
        if (!Renderer.AcquireTextureMapResources(&cubeMap))
        {
            m_logger.Error("Load() - Unable to acquire resources for cube map texture.");
            return false;
        }

        // Acquire our skybox texture
        cubeMap.texture = Textures.AcquireCube(m_config.cubeMapName.Data(), true);

        // Build the cube geometry based on the config
        g = Geometric.AcquireFromConfig(m_skyboxGeometryConfig, true);
        frameNumber = INVALID_ID_U64;

        // Get our builtin skybox shader
        // TODO: Allow configurable shader here
        const auto shader = Shaders.Get("Shader.Builtin.Skybox");
        TextureMap* maps[1] = {&cubeMap};

        // Acquire our shader instance resources
        if (!Renderer.AcquireShaderInstanceResources(shader, maps, &instanceId))
        {
            m_logger.Error("Load() - Unable to acquire shader resources for skybox texture.");
            return false;
        }

        return true;
    }

    bool Skybox::Unload()
    {
        // TODO: Allow configurable shader here
        const auto shader = Shaders.Get("Shader.Builtin.Skybox");

        Renderer.ReleaseShaderInstanceResources(shader, instanceId);
        instanceId = INVALID_ID;

        Renderer.ReleaseTextureMapResources(&cubeMap);
        frameNumber = INVALID_ID_U64;

        Geometric.DisposeConfig(m_skyboxGeometryConfig);

        Geometric.Release(g);

        return true;
    }

    void Skybox::Destroy()
    {
        if (instanceId != INVALID_ID)
        {
            // The skybox is loaded so we should unload first
            if (!Unload())
            {
                m_logger.Error("Destroy() - Failed to unload the skybox before destroying");
                return;
            }
        }

        g = nullptr;
    }
}  // namespace C3D
