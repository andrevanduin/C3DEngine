
#include "render_view_world.h"

#include "core/engine.h"
#include "math/c3d_math.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_types.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "resources/textures/texture.h"
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
        const auto debugShaderName    = "Shader.Builtin.Color3DShader";

        ShaderConfig shaderConfig;
        if (!Resources.Load(materialShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource for Material Shader");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create {}", materialShaderName);
            return false;
        }

        Resources.Unload(shaderConfig);

        if (!Resources.Load(terrainShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource for Terrain Shader.");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create '{}'.", terrainShaderName);
            return false;
        }

        Resources.Unload(shaderConfig);

        if (!Resources.Load(debugShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource for Debug Shader.");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create '{}'.", debugShaderName);
            return false;
        }

        Resources.Unload(shaderConfig);

        m_materialShader = Shaders.Get(m_customShaderName ? m_customShaderName.Data() : materialShaderName);
        m_terrainShader  = Shaders.Get(terrainShaderName);
        m_debugShader    = Shaders.Get(debugShaderName);

        m_debugShaderLocations.projection = m_debugShader->GetUniformIndex("projection");
        m_debugShaderLocations.view       = m_debugShader->GetUniformIndex("view");
        m_debugShaderLocations.model      = m_debugShader->GetUniformIndex("model");

        if (!m_materialShader)
        {
            m_logger.Error("Load() - Failed to get Material Shader.");
            return false;
        }

        if (!m_terrainShader)
        {
            m_logger.Error("Load() - Failed to get Terrain Shader.");
            return false;
        }

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
        outPacket->debugGeometries.SetAllocator(frameAllocator);

        m_distances.SetAllocator(frameAllocator);

        for (const auto& gData : worldData.worldGeometries)
        {
            auto hasTransparency = false;
            if (gData.geometry->material->type == MaterialType::Phong)
            {
                // NOTE: Since this is a Phong material we know that the first map will be the diffuse
                hasTransparency =
                    (gData.geometry->material->maps[0].texture->flags & TextureFlag::HasTransparency) == 0;
            }

            if (hasTransparency)
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

        for (auto& debug : worldData.debugGeometries)
        {
            outPacket->debugGeometries.PushBack(debug);
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
                m_logger.Error("OnRender() - BeginRenderPass failed for pass width id '{}'.", pass->id);
                return false;
            }

            // Terrain geometries
            if (!packet->terrainGeometries.Empty())
            {
                if (!Shaders.UseById(m_terrainShader->id))
                {
                    m_logger.Error("OnRender() - Failed to use shader: '{}'.", m_terrainShader->name);
                    return false;
                }

                if (!Materials.ApplyGlobal(m_terrainShader->id, frameNumber, &packet->projectionMatrix,
                                           &packet->viewMatrix, &packet->ambientColor, &packet->viewPosition,
                                           m_renderMode))
                {
                    m_logger.Error("OnRender() - Failed to apply globals for shader: '{}'.", m_terrainShader->name);
                    return false;
                }

                for (auto& terrain : packet->terrainGeometries)
                {
                    if (terrain.geometry->id == INVALID_ID) continue;

                    Material* mat =
                        terrain.geometry->material ? terrain.geometry->material : Materials.GetDefaultTerrain();

                    // Check if this material has already been updated this frame to avoid unnecessary updates.
                    // This means we always bind the shader but we skip updating the internal shader bindings if it has
                    // already happened this frame for this material (for example because the previous terrain geometry
                    // uses the same material)
                    bool needsUpdate = mat->renderFrameNumber != frameNumber;
                    if (!Materials.ApplyInstance(mat, needsUpdate))
                    {
                        m_logger.Warn("OnRender() - Failed to apply instance for shader: '{}'.", m_terrainShader->name);
                        continue;
                    }
                    // Sync the frame number with the current
                    mat->renderFrameNumber = frameNumber;

                    Materials.ApplyLocal(mat, &terrain.model);
                    Renderer.DrawGeometry(terrain);
                }
            }

            // Static geometries
            if (!packet->geometries.Empty())
            {
                if (!Shaders.UseById(m_materialShader->id))
                {
                    m_logger.Error("OnRender() - Failed to use shader: '{}'.", m_materialShader->name);
                    return false;
                }

                // TODO: Generic way to request data such as ambient color (which should come from a scene)
                if (!Materials.ApplyGlobal(m_materialShader->id, frameNumber, &packet->projectionMatrix,
                                           &packet->viewMatrix, &packet->ambientColor, &packet->viewPosition,
                                           m_renderMode))
                {
                    m_logger.Error("OnRender() - Failed to apply globals for shader: '{}'", m_materialShader->name);
                    return false;
                }

                for (auto& geometry : packet->geometries)
                {
                    Material* mat = geometry.geometry->material ? geometry.geometry->material : Materials.GetDefault();

                    const bool needsUpdate = mat->renderFrameNumber != frameNumber;
                    if (!Materials.ApplyInstance(mat, needsUpdate))
                    {
                        m_logger.Warn("Failed to apply material '{}'. Skipping draw.", mat->name);
                        continue;
                    }
                    // Sync the frame number with the current
                    mat->renderFrameNumber = static_cast<u32>(frameNumber);

                    Materials.ApplyLocal(mat, &geometry.model);
                    Renderer.DrawGeometry(geometry);
                }
            }

            // Debug geometries
            if (!packet->debugGeometries.Empty())
            {
                if (!Shaders.UseById(m_debugShader->id))
                {
                    m_logger.Error("OnRender() - Failed to use shader: '{}'.", m_debugShader->name);
                    return false;
                }

                // Globals
                Shaders.SetUniformByIndex(m_debugShaderLocations.projection, &packet->projectionMatrix);
                Shaders.SetUniformByIndex(m_debugShaderLocations.view, &packet->viewMatrix);

                Shaders.ApplyGlobal();

                for (auto& debug : packet->debugGeometries)
                {
                    // NOTE: No instance-level uniforms required

                    // Set local
                    Shaders.SetUniformByIndex(m_debugShaderLocations.model, &debug.model);

                    // Draw it
                    Renderer.DrawGeometry(debug);
                }
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
