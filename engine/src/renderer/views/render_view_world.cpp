
#include "render_view_world.h"

#include "core/engine.h"
#include "math/c3d_math.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_types.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "systems/cameras/camera_system.h"
#include "systems/events/event_system.h"
#include "systems/lights/light_system.h"
#include "systems/materials/material_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D
{
    RenderViewWorld::RenderViewWorld(const RenderViewConfig& config)
        : RenderView(ToUnderlying(RenderViewKnownType::World), config)
    {}

    bool RenderViewWorld::OnCreate()
    {
        // Builtin skybox shader
        const auto materialShaderName = "Shader.Builtin.Material";
        const auto terrainShaderName  = "Shader.Builtin.Terrain";

        ShaderResource res;
        if (!Resources.Load(materialShaderName, res))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource for Material Shader");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], res.config))
        {
            m_logger.Error("OnCreate() - Failed to create {}", materialShaderName);
            return false;
        }

        Resources.Unload(res);

        if (!Resources.Load(terrainShaderName, res))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource for Terrain Shader");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], res.config))
        {
            m_logger.Error("OnCreate() - Failed to create {}", terrainShaderName);
            return false;
        }

        Resources.Unload(res);

        m_materialShader = Shaders.Get(m_customShaderName ? m_customShaderName.Data() : materialShaderName);
        m_terrainShader  = Shaders.Get(terrainShaderName);

        if (!m_materialShader)
        {
            m_logger.Error("Load() - Failed to get Material Shader");
            return false;
        }

        if (!m_terrainShader)
        {
            m_logger.Error("Load() - Failed to get Terrain Shader");
            return false;
        }

        // Acquire locations
        m_terrainShaderLocations.projection   = m_terrainShader->GetUniformIndex("projection");
        m_terrainShaderLocations.view         = m_terrainShader->GetUniformIndex("view");
        m_terrainShaderLocations.ambientColor = m_terrainShader->GetUniformIndex("ambientColor");
        m_terrainShaderLocations.diffuseColor = m_terrainShader->GetUniformIndex("diffuseColor");
        m_terrainShaderLocations.viewPosition = m_terrainShader->GetUniformIndex("viewPosition");
        m_terrainShaderLocations.shininess    = m_terrainShader->GetUniformIndex("shininess");
        // m_terrainShaderLocations.diffuseTexture  = m_terrainShader->GetUniformIndex("diffuseTexture");
        // m_terrainShaderLocations.specularTexture = m_terrainShader->GetUniformIndex("specularTexture");
        // m_terrainShaderLocations.normalTexture   = m_terrainShader->GetUniformIndex("normalTexture");
        m_terrainShaderLocations.model      = m_terrainShader->GetUniformIndex("model");
        m_terrainShaderLocations.renderMode = m_terrainShader->GetUniformIndex("mode");
        m_terrainShaderLocations.dirLight   = m_terrainShader->GetUniformIndex("dirLight");
        m_terrainShaderLocations.pLights    = m_terrainShader->GetUniformIndex("pLights");
        m_terrainShaderLocations.numPLights = m_terrainShader->GetUniformIndex("numPLights");

        const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
        m_projectionMatrix     = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);

        m_camera = Cam.GetDefault();

        // TODO: Obtain from scene
        m_ambientColor = vec4(0.25f, 0.25f, 0.25f, 1.0f);

        // Register our render mode change event listener
        m_onEventCallback =
            Event.Register(EventCodeSetRenderMode, [this](const u16 code, void* sender, const EventContext& context) {
                return OnEvent(code, sender, context);
            });
        return true;
    }

    void RenderViewWorld::OnDestroy()
    {
        RenderView::OnDestroy();
        Event.Unregister(m_onEventCallback);
    }

    void RenderViewWorld::OnResize()
    {
        const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
        m_projectionMatrix     = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);
    }

    bool RenderViewWorld::OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket)
    {
        if (!data || !outPacket)
        {
            m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
            return false;
        }

        const auto& worldData = *static_cast<RenderViewWorldData*>(data);

        outPacket->view             = this;
        outPacket->projectionMatrix = m_projectionMatrix;
        outPacket->viewMatrix       = m_camera->GetViewMatrix();
        outPacket->viewPosition     = m_camera->GetPosition();
        outPacket->ambientColor     = m_ambientColor;

        outPacket->geometries.SetAllocator(frameAllocator);
        outPacket->terrainGeometries.SetAllocator(frameAllocator);

        m_distances.SetAllocator(frameAllocator);

        for (const auto& gData : worldData.worldGeometries)
        {
            if ((gData.geometry->material->diffuseMap.texture->flags & TextureFlag::HasTransparency) == 0)
            {
                // Material has no transparency
                outPacket->geometries.PushBack(gData);
            }
            else
            {
                vec3 center        = vec4(gData.geometry->center, 1.0f) * gData.model;
                const f32 distance = glm::distance(center, m_camera->GetPosition());

                GeometryDistance gDistance{ gData, Abs(distance) };
                m_distances.PushBack(gDistance);
            }
        }

        std::ranges::sort(m_distances,
                          [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance < b.distance; });

        for (auto& [g, distance] : m_distances)
        {
            outPacket->geometries.PushBack(g);
        }

        for (auto& terrain : worldData.terrainGeometries)
        {
            outPacket->terrainGeometries.PushBack(terrain);
        }

        m_distances.Clear();
        return true;
    }

    bool RenderViewWorld::OnRender(const FrameData& frameData, const RenderViewPacket* packet, const u64 frameNumber,
                                   const u64 renderTargetIndex)
    {
        for (const auto pass : passes)
        {
            if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
            {
                m_logger.Error("OnRender() - BeginRenderPass failed for pass width id '{}'", pass->id);
                return false;
            }

            for (auto& terrain : packet->terrainGeometries)
            {
                if (!Shaders.UseById(m_terrainShader->id))
                {
                    m_logger.Error("Render() - Failed to use shader with id: '{}'", m_terrainShader->id);
                    return false;
                }

                // Apply uniforms, all are global for terrains
                Shaders.SetUniformByIndex(m_terrainShaderLocations.projection, &packet->projectionMatrix);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.view, &packet->viewMatrix);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.ambientColor, &packet->ambientColor);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.viewPosition, &packet->viewPosition);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.renderMode, &m_renderMode);

                // TODO: Change hardcoded diffuse color
                static vec4 white = vec4(1.0);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.diffuseColor, &white);
                // Shaders.SetSamplerByIndex(m_shaderLocations.diffuseTexture, &diffuseTexture);
                // Shaders.SetSamplerByIndex(m_shaderLocations.specularTexture, &specularTexture);
                // Shaders.SetSamplerByIndex(m_shaderLocations.normalTexture, &normalTexture);
                // TODO: Change hardcoded shininess
                static f32 shininess = 32.0f;
                Shaders.SetUniformByIndex(m_terrainShaderLocations.shininess, &shininess);

                const auto dirLight    = Lights.GetDirectionalLight();
                const auto pointLights = Lights.GetPointLights();
                const auto numPLights  = pointLights.Size();

                Shaders.SetUniformByIndex(m_terrainShaderLocations.dirLight, &dirLight->data);
                Shaders.SetUniformByIndex(m_terrainShaderLocations.pLights, pointLights.GetData());
                Shaders.SetUniformByIndex(m_terrainShaderLocations.numPLights, &numPLights);

                Shaders.SetUniformByIndex(m_terrainShaderLocations.model, &terrain.model);

                Shaders.ApplyGlobal();
                Renderer.DrawTerrainGeometry(terrain);
            }

            const auto materialShaderId = m_materialShader->id;

            if (!Shaders.UseById(materialShaderId))
            {
                m_logger.Error("OnRender() - Failed to use shader with id {}", materialShaderId);
                return false;
            }

            // TODO: Generic way to request data such as ambient color (which should come from a scene)
            if (!Materials.ApplyGlobal(materialShaderId, frameNumber, &packet->projectionMatrix, &packet->viewMatrix,
                                       &packet->ambientColor, &packet->viewPosition, m_renderMode))
            {
                m_logger.Error("OnRender() - Failed to apply globals for shader with id {}", materialShaderId);
                return false;
            }

            for (auto& geometry : packet->geometries)
            {
                Material* m = geometry.geometry->material ? geometry.geometry->material : Materials.GetDefault();

                const bool needsUpdate = m->renderFrameNumber != frameNumber;
                if (!Materials.ApplyInstance(m, needsUpdate))
                {
                    m_logger.Warn("Failed to apply material '{}'. Skipping draw.", m->name);
                    continue;
                }

                // Sync the frame number with the current
                m->renderFrameNumber = static_cast<u32>(frameNumber);

                Materials.ApplyLocal(m, &geometry.model);

                Renderer.DrawGeometry(geometry);
            }

            if (!Renderer.EndRenderPass(pass))
            {
                m_logger.Error("OnRender() - EndRenderPass failed for pass with id '{}'", pass->id);
                return false;
            }
        }

        return true;
    }

    bool RenderViewWorld::OnEvent(const u16 code, void* sender, const EventContext& context)
    {
        if (code == EventCodeSetRenderMode)
        {
            switch (const i32 mode = context.data.i32[0])
            {
                case RendererViewMode::Default:
                    m_logger.Debug("Renderer mode set to default");
                    m_renderMode = RendererViewMode::Default;
                    break;
                case RendererViewMode::Lighting:
                    m_logger.Debug("Renderer mode set to lighting");
                    m_renderMode = RendererViewMode::Lighting;
                    break;
                case RendererViewMode::Normals:
                    m_logger.Debug("Renderer mode set to normals");
                    m_renderMode = RendererViewMode::Normals;
                    break;
                default:
                    m_logger.Fatal("Unknown render mode");
                    break;
            }
        }

        return false;
    }
}  // namespace C3D
