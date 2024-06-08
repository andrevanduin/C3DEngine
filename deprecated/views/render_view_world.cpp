
#include "render_view_world.h"

#include <core/engine.h>
#include <math/c3d_math.h>
#include <renderer/renderer_frontend.h>
#include <renderer/renderer_types.h>
#include <renderer/viewport.h>
#include <resources/loaders/shader_loader.h>
#include <resources/mesh.h>
#include <resources/textures/texture.h>
#include <systems/cameras/camera_system.h>
#include <systems/events/event_system.h>
#include <systems/lights/light_system.h>
#include <systems/materials/material_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>

constexpr const char* INSTANCE_NAME = "RENDER_VIEW_WORLD";

RenderViewWorld::RenderViewWorld() : RenderView("WORLD_VIEW", "") {}

void RenderViewWorld::OnSetupPasses()
{
    {
        C3D::RenderPassConfig pass{};
        pass.name       = "RenderPass.Builtin.Skybox";
        pass.clearColor = { 0, 0, 0.2f, 1.0f };
        pass.clearFlags = C3D::ClearColorBuffer;
        pass.depth      = 1.0f;
        pass.stencil    = 0;

        C3D::RenderTargetAttachmentConfig targetAttachment = {};
        targetAttachment.type                              = C3D::RenderTargetAttachmentType::Color;
        targetAttachment.source                            = C3D::RenderTargetAttachmentSource::Default;
        targetAttachment.loadOperation                     = C3D::RenderTargetAttachmentLoadOperation::DontCare;
        targetAttachment.storeOperation                    = C3D::RenderTargetAttachmentStoreOperation::Store;
        targetAttachment.presentAfter                      = false;

        pass.target.attachments.PushBack(targetAttachment);

        pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

        m_passConfigs.PushBack(pass);
    }
    {
        C3D::RenderPassConfig pass;
        pass.name       = "RenderPass.Builtin.World";
        pass.clearColor = { 0, 0, 0.2f, 1.0f };
        pass.clearFlags = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
        pass.depth      = 1.0f;
        pass.stencil    = 0;

        C3D::RenderTargetAttachmentConfig targetAttachments[2]{};
        targetAttachments[0].type           = C3D::RenderTargetAttachmentType::Color;
        targetAttachments[0].source         = C3D::RenderTargetAttachmentSource::Default;
        targetAttachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
        targetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
        targetAttachments[0].presentAfter   = false;

        targetAttachments[1].type           = C3D::RenderTargetAttachmentType::Depth;
        targetAttachments[1].source         = C3D::RenderTargetAttachmentSource::Default;
        targetAttachments[1].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
        targetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
        targetAttachments[1].presentAfter   = false;

        pass.target.attachments.PushBack(targetAttachments[0]);
        pass.target.attachments.PushBack(targetAttachments[1]);

        pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

        m_passConfigs.PushBack(pass);
    }
}

bool RenderViewWorld::OnCreate()
{
    // Builtin skybox shader
    const auto materialShaderName = "Shader.Builtin.Material";
    const auto terrainShaderName  = "Shader.Builtin.Terrain";
    const auto debugShaderName    = "Shader.Builtin.Color3D";
    const auto skyboxShaderName   = "Shader.Builtin.Skybox";

    C3D::ShaderConfig shaderConfig;
    if (!Resources.Load(materialShaderName, shaderConfig))
    {
        ERROR_LOG("Failed to load ShaderResource for Material Shader.");
        return false;
    }
    // NOTE: Second pass is the WORLD pass
    if (!Shaders.Create(m_passes[1], shaderConfig))
    {
        ERROR_LOG("Failed to create: '{}'.", materialShaderName);
        return false;
    }
    Resources.Unload(shaderConfig);

    if (!Resources.Load(terrainShaderName, shaderConfig))
    {
        ERROR_LOG("Failed to load ShaderResource for Terrain Shader.");
        return false;
    }
    // NOTE: Second pass is the WORLD pass
    if (!Shaders.Create(m_passes[1], shaderConfig))
    {
        ERROR_LOG("Failed to create: '{}'.", terrainShaderName);
        return false;
    }
    Resources.Unload(shaderConfig);

    if (!Resources.Load(debugShaderName, shaderConfig))
    {
        ERROR_LOG("Failed to load ShaderResource for Debug Shader.");
        return false;
    }
    // NOTE: Second pass is the WORLD pass
    if (!Shaders.Create(m_passes[1], shaderConfig))
    {
        ERROR_LOG("Failed to create: '{}'.", debugShaderName);
        return false;
    }
    Resources.Unload(shaderConfig);

    if (!Resources.Load(skyboxShaderName, shaderConfig))
    {
        ERROR_LOG("Failed to load ShaderResource for Skybox Shader.");
        return false;
    }
    // NOTE: First pass is the Skybox pass
    if (!Shaders.Create(m_passes[0], shaderConfig))
    {
        ERROR_LOG("Failed to load builtin Skybox Shader.");
        return false;
    }
    Resources.Unload(shaderConfig);

    m_materialShader = Shaders.Get(m_customShaderName ? m_customShaderName.Data() : materialShaderName);
    m_terrainShader  = Shaders.Get(terrainShaderName);
    m_debugShader    = Shaders.Get(debugShaderName);
    m_skyboxShader   = Shaders.Get(skyboxShaderName);

    if (!m_materialShader)
    {
        ERROR_LOG("Failed to get Material Shader.");
        return false;
    }

    if (!m_terrainShader)
    {
        ERROR_LOG("Failed to get Terrain Shader.");
        return false;
    }

    if (!m_debugShader)
    {
        ERROR_LOG("LFailed to get Debug Shader.");
        return false;
    }

    if (!m_skyboxShader)
    {
        ERROR_LOG("Failed to get Skybox Shader.");
        return false;
    }

    m_debugShaderLocations.projection = m_debugShader->GetUniformIndex("projection");
    m_debugShaderLocations.view       = m_debugShader->GetUniformIndex("view");
    m_debugShaderLocations.model      = m_debugShader->GetUniformIndex("model");

    m_skyboxShaderLocations.projection = m_skyboxShader->GetUniformIndex("projection");
    m_skyboxShaderLocations.view       = m_skyboxShader->GetUniformIndex("view");
    m_skyboxShaderLocations.cubeMap    = m_skyboxShader->GetUniformIndex("cubeTexture");

    // TODO: Obtain from scene
    m_ambientColor = vec4(0.25f, 0.25f, 0.25f, 1.0f);

    // Register our render mode change event listener
    m_onEventCallback = Event.Register(C3D::EventCodeSetRenderMode, [this](const u16 code, void* sender, const C3D::EventContext& context) {
        return OnEvent(code, sender, context);
    });
    return true;
}

void RenderViewWorld::OnDestroy()
{
    RenderView::OnDestroy();
    Event.Unregister(m_onEventCallback);
}

bool RenderViewWorld::OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                                    C3D::RenderViewPacket* outPacket)
{
    if (!data || !outPacket)
    {
        WARN_LOG("Requires a valid pointer to data and outPacket.");
        return false;
    }

    const auto& worldData = *static_cast<RenderViewWorldData*>(data);

    outPacket->view             = this;
    outPacket->projectionMatrix = viewport.GetProjection();
    outPacket->viewMatrix       = camera->GetViewMatrix();
    outPacket->viewPosition     = camera->GetPosition();
    outPacket->ambientColor     = m_ambientColor;
    outPacket->viewport         = &viewport;

    outPacket->geometries.SetAllocator(frameData.allocator);
    outPacket->terrainGeometries.SetAllocator(frameData.allocator);
    outPacket->debugGeometries.SetAllocator(frameData.allocator);

    outPacket->skyboxData = worldData.skyboxData;

    m_distances.SetAllocator(frameData.allocator);

    for (const auto& gData : worldData.worldGeometries)
    {
        auto hasTransparency = false;
        if (gData.geometry->material->type == C3D::MaterialType::Phong)
        {
            // NOTE: Since this is a Phong material we know that the first map will be the diffuse
            hasTransparency = (gData.geometry->material->maps[0].texture->flags & C3D::TextureFlag::HasTransparency) == 0;
        }

        if (hasTransparency)
        {
            // Material has no transparency
            outPacket->geometries.PushBack(gData);
        }
        else
        {
            vec3 center        = vec4(gData.geometry->center, 1.0f) * gData.model;
            const f32 distance = glm::distance(center, camera->GetPosition());

            GeometryDistance gDistance{ gData, C3D::Abs(distance) };
            m_distances.PushBack(gDistance);
        }
    }

    std::ranges::sort(m_distances, [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance < b.distance; });

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

bool RenderViewWorld::OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet)
{
    // Bind the viewport
    Renderer.SetActiveViewport(packet->viewport);

    {
        // Skybox pass
        const auto pass = m_passes[0];
        if (!Renderer.BeginRenderPass(pass, &pass->targets[frameData.renderTargetIndex]))
        {
            ERROR_LOG("BeginRenderPass failed for pass width id: '{}'.", pass->id);
            return false;
        }

        if (packet->skyboxData.box)
        {
            Shaders.UseById(m_skyboxShader->id);

            // Get the view matrix, but zero out the position so the skybox stays put on screen.
            mat4 viewMatrix  = packet->viewMatrix;
            viewMatrix[3][0] = 0.0f;
            viewMatrix[3][1] = 0.0f;
            viewMatrix[3][2] = 0.0f;

            // Apply our globals
            Renderer.BindShaderGlobals(*m_skyboxShader);

            if (!Shaders.SetUniformByIndex(m_skyboxShaderLocations.projection, &packet->projectionMatrix))
            {
                ERROR_LOG("Failed to apply Skybox projection uniform.");
                return false;
            }

            if (!Shaders.SetUniformByIndex(m_skyboxShaderLocations.view, &viewMatrix))
            {
                ERROR_LOG("Failed to apply Skybox view uniform.");
                return false;
            }
            Shaders.ApplyGlobal(true);

            // Instance
            Shaders.BindInstance(packet->skyboxData.box->instanceId);
            if (!Shaders.SetUniformByIndex(m_skyboxShaderLocations.cubeMap, &packet->skyboxData.box->cubeMap))
            {
                ERROR_LOG("Failed to apply Skybox cubemap uniform.");
                return false;
            }

            bool needsUpdate =
                packet->skyboxData.box->frameNumber != frameData.frameNumber || packet->skyboxData.box->drawIndex != frameData.drawIndex;
            Shaders.ApplyInstance(needsUpdate);

            // Sync the frame number and draw index
            packet->skyboxData.box->frameNumber = frameData.frameNumber;
            packet->skyboxData.box->drawIndex   = frameData.drawIndex;

            // Draw it
            const auto renderData = C3D::GeometryRenderData(packet->skyboxData.box->g);
            Renderer.DrawGeometry(renderData);
        }

        if (!Renderer.EndRenderPass(pass))
        {
            ERROR_LOG("EndRenderPass failed for pass with id: '{}'", pass->id);
        }
    }

    {
        // World pass
        const auto pass = m_passes[1];
        if (!Renderer.BeginRenderPass(pass, &pass->targets[frameData.renderTargetIndex]))
        {
            ERROR_LOG("BeginRenderPass failed for pass width id: '{}'.", pass->id);
            return false;
        }

        const auto frameNumber = frameData.frameNumber;
        const auto drawIndex   = frameData.drawIndex;

        // Terrain geometries
        if (!packet->terrainGeometries.Empty())
        {
            if (!Shaders.UseById(m_terrainShader->id))
            {
                ERROR_LOG("Failed to use shader: '{}'.", m_terrainShader->name);
                return false;
            }

            if (!Materials.ApplyGlobal(m_terrainShader->id, frameData, &packet->projectionMatrix, &packet->viewMatrix,
                                       &packet->ambientColor, &packet->viewPosition, m_renderMode))
            {
                ERROR_LOG("Failed to apply globals for shader: '{}'.", m_terrainShader->name);
                return false;
            }

            for (auto& terrain : packet->terrainGeometries)
            {
                C3D::Material* mat = terrain.geometry->material ? terrain.geometry->material : Materials.GetDefaultTerrain();

                // Check if this material has already been updated this frame to avoid unnecessary updates.
                // This means we always bind the shader but we skip updating the internal shader bindings if it has
                // already happened this frame for this material (for example because the previous terrain geometry
                // uses the same material)
                bool needsUpdate = mat->renderFrameNumber != frameData.frameNumber || mat->renderDrawIndex != frameData.drawIndex;
                if (!Materials.ApplyInstance(mat, frameData, needsUpdate))
                {
                    WARN_LOG("Failed to apply instance for shader: '{}'.", m_terrainShader->name);
                    continue;
                }
                // Sync the frame number and draw index with the current
                mat->renderFrameNumber = frameData.frameNumber;
                mat->renderDrawIndex   = frameData.drawIndex;

                // Apply the locals for this geometry
                Materials.ApplyLocal(mat, &terrain.model);
                // Actually draw our geometry
                Renderer.DrawGeometry(terrain);
            }
        }

        // Static geometries
        if (!packet->geometries.Empty())
        {
            if (!Shaders.UseById(m_materialShader->id))
            {
                ERROR_LOG("Failed to use shader: '{}'.", m_materialShader->name);
                return false;
            }

            // TODO: Generic way to request data such as ambient color (which should come from a scene)
            if (!Materials.ApplyGlobal(m_materialShader->id, frameData, &packet->projectionMatrix, &packet->viewMatrix,
                                       &packet->ambientColor, &packet->viewPosition, m_renderMode))
            {
                ERROR_LOG("Failed to apply globals for shader: '{}'.", m_materialShader->name);
                return false;
            }

            for (auto& geometry : packet->geometries)
            {
                C3D::Material* mat = geometry.geometry->material ? geometry.geometry->material : Materials.GetDefault();

                const bool needsUpdate = mat->renderFrameNumber != frameNumber;
                if (!Materials.ApplyInstance(mat, frameData, needsUpdate))
                {
                    WARN_LOG("Failed to apply material: '{}'. Skipping draw.", mat->name);
                    continue;
                }
                // Sync the frame number with the current
                mat->renderFrameNumber = static_cast<u32>(frameNumber);

                // Apply the locals for this geometry
                Materials.ApplyLocal(mat, &geometry.model);

                // Actually draw our geometry
                Renderer.DrawGeometry(geometry);
            }
        }

        // Debug geometries
        if (!packet->debugGeometries.Empty())
        {
            if (!Shaders.UseById(m_debugShader->id))
            {
                ERROR_LOG("Failed to use shader: '{}'.", m_debugShader->name);
                return false;
            }

            // Globals
            Shaders.SetUniformByIndex(m_debugShaderLocations.projection, &packet->projectionMatrix);
            Shaders.SetUniformByIndex(m_debugShaderLocations.view, &packet->viewMatrix);

            Shaders.ApplyGlobal(true);

            for (auto& debug : packet->debugGeometries)
            {
                // NOTE: No instance-level uniforms required

                // Set local
                Shaders.SetUniformByIndex(m_debugShaderLocations.model, &debug.model);

                // Draw it
                Renderer.DrawGeometry(debug);
            }

            // HACK: This should be handled every frame, by the shader system
            m_debugShader->frameNumber = frameNumber;
        }

        if (!Renderer.EndRenderPass(pass))
        {
            ERROR_LOG("EndRenderPass failed for pass with id: '{}'.", pass->id);
            return false;
        }
    }

    return true;
}

bool RenderViewWorld::OnEvent(const u16 code, void* sender, const C3D::EventContext& context)
{
    if (code == C3D::EventCodeSetRenderMode)
    {
        switch (const i32 mode = context.data.i32[0])
        {
            case C3D::RendererViewMode::Default:
                DEBUG_LOG("Renderer mode set to default.");
                m_renderMode = C3D::RendererViewMode::Default;
                break;
            case C3D::RendererViewMode::Lighting:
                DEBUG_LOG("Renderer mode set to lighting.");
                m_renderMode = C3D::RendererViewMode::Lighting;
                break;
            case C3D::RendererViewMode::Normals:
                DEBUG_LOG("Renderer mode set to normals.");
                m_renderMode = C3D::RendererViewMode::Normals;
                break;
            case C3D::RendererViewMode::Cascades:
                DEBUG_LOG("Renderer mode set to cascades.");
                m_renderMode = C3D::RendererViewMode::Cascades;
                break;
            default:
                FATAL_LOG("Unknown render mode.");
                break;
        }
    }

    return false;
}
