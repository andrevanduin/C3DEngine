
#include "render_view_wireframe.h"

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

constexpr const char* INSTANCE_NAME = "RENDER_VIEW_WIREFRAME";

RenderViewWireframe::RenderViewWireframe() : RenderView("WIREFRAME_VIEW", "") {}

void RenderViewWireframe::OnSetupPasses()
{
    C3D::RenderPassConfig pass{};
    pass.name       = "RenderPass.Builtin.Wireframe";
    pass.clearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearColorBuffer | C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig colorAttachment = {};
    colorAttachment.type                              = C3D::RenderTargetAttachmentType::Color;
    colorAttachment.source                            = C3D::RenderTargetAttachmentSource::Default;
    colorAttachment.loadOperation                     = C3D::RenderTargetAttachmentLoadOperation::Load;
    colorAttachment.storeOperation                    = C3D::RenderTargetAttachmentStoreOperation::Store;
    colorAttachment.presentAfter                      = false;

    C3D::RenderTargetAttachmentConfig depthAttachment = {};
    depthAttachment.type                              = C3D::RenderTargetAttachmentType::Depth;
    depthAttachment.source                            = C3D::RenderTargetAttachmentSource::Default;
    depthAttachment.loadOperation                     = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    depthAttachment.storeOperation                    = C3D::RenderTargetAttachmentStoreOperation::Store;
    depthAttachment.presentAfter                      = false;

    pass.target.attachments.PushBack(colorAttachment);
    pass.target.attachments.PushBack(depthAttachment);

    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    m_passConfigs.PushBack(pass);
}

bool RenderViewWireframe::OnCreate()
{
    const auto meshShaderName    = "Shader.Builtin.Wireframe";
    const auto terrainShaderName = "Shader.Builtin.WireframeTerrain";

    const char* shaderNames[2]          = { meshShaderName, terrainShaderName };
    const vec4 normalColors[2]          = { vec4(0.5f, 0.8f, 0.8f, 1.0f), vec4(0.8f, 0.8f, 0.5f, 1.0f) };
    WireframeShaderInfo* shaderInfos[2] = { &m_meshShader, &m_terrainShader };

    for (u8 i = 0; i < 2; i++)
    {
        const auto currentName = shaderNames[i];
        auto currentInfo       = shaderInfos[i];

        C3D::ShaderConfig shaderConfig;
        if (!Resources.Load(currentName, shaderConfig))
        {
            ERROR_LOG("Failed to load ShaderResource for: '{}'.", currentName);
            return false;
        }
        if (!Shaders.Create(m_passes[0], shaderConfig))
        {
            ERROR_LOG("Failed to create: '{}'.", currentName);
            return false;
        }
        Resources.Unload(shaderConfig);

        currentInfo->shader = Shaders.Get(currentName);
        if (!currentInfo->shader)
        {
            ERROR_LOG("Failed to get: '{}' Shader.", currentName);
            return false;
        }

        // Get shader locations
        currentInfo->locations.projection = currentInfo->shader->GetUniformIndex("projection");
        currentInfo->locations.view       = currentInfo->shader->GetUniformIndex("view");
        currentInfo->locations.model      = currentInfo->shader->GetUniformIndex("model");
        currentInfo->locations.color      = currentInfo->shader->GetUniformIndex("color");

        // Obtain shader instance resources
        currentInfo->normalInstance.color = normalColors[i];
        if (!Renderer.AcquireShaderInstanceResources(*currentInfo->shader, 0, nullptr, &currentInfo->normalInstance.id))
        {
            ERROR_LOG("Unable to Acquire Geometry Shader Instance Resources from Wireframe Shader.");
            return false;
        }

        currentInfo->selectedInstance.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        if (!Renderer.AcquireShaderInstanceResources(*currentInfo->shader, 0, nullptr, &currentInfo->selectedInstance.id))
        {
            ERROR_LOG("Unable to Acquire Geometry Shader Instance Resources from Wireframe Shader.");
            return false;
        }
    }

    return true;
}

void RenderViewWireframe::OnDestroy()
{
    RenderView::OnDestroy();

    // Release the shader instance resources for the mesh wireframe shader
    Renderer.ReleaseShaderInstanceResources(*m_meshShader.shader, m_meshShader.normalInstance.id);
    Renderer.ReleaseShaderInstanceResources(*m_meshShader.shader, m_meshShader.selectedInstance.id);

    // Release the shader instance resources for the terrain wireframe shader
    Renderer.ReleaseShaderInstanceResources(*m_terrainShader.shader, m_terrainShader.normalInstance.id);
    Renderer.ReleaseShaderInstanceResources(*m_terrainShader.shader, m_terrainShader.selectedInstance.id);
}

bool RenderViewWireframe::OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                                        C3D::RenderViewPacket* outPacket)
{
    if (!data || !outPacket)
    {
        WARN_LOG("Requires a valid pointer to data and outPacket.");
        return false;
    }

    const auto& worldData = *static_cast<RenderViewWireframeData*>(data);

    outPacket->view             = this;
    outPacket->projectionMatrix = viewport.GetProjection();
    outPacket->viewMatrix       = camera->GetViewMatrix();
    outPacket->viewPosition     = camera->GetPosition();
    outPacket->viewport         = &viewport;

    // Set frame allocator for our outpacket dynamic arrays
    outPacket->geometries.SetAllocator(frameData.frameAllocator);
    outPacket->terrainGeometries.SetAllocator(frameData.frameAllocator);
    outPacket->debugGeometries.SetAllocator(frameData.frameAllocator);

    // Reset draw indices
    m_meshShader.normalInstance.drawIndex      = 0;
    m_meshShader.selectedInstance.drawIndex    = 0;
    m_terrainShader.normalInstance.drawIndex   = 0;
    m_terrainShader.selectedInstance.drawIndex = 0;

    m_selectedId = worldData.selectedId;

    // Iterate our world geometries
    for (const auto& gData : worldData.worldGeometries)
    {
        outPacket->geometries.EmplaceBack(gData.model, gData.geometry, gData.uniqueId, gData.windingInverted);
    }

    // Iterate our terrain geometries
    for (const auto& terrainData : worldData.terrainGeometries)
    {
        outPacket->terrainGeometries.EmplaceBack(terrainData.model, terrainData.geometry, terrainData.uniqueId,
                                                 terrainData.windingInverted);
    }

    return true;
}

bool RenderViewWireframe::OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet)
{
    // Bind the viewport
    Renderer.SetActiveViewport(packet->viewport);

    // There is only one pass
    const auto pass = m_passes[0];
    if (!Renderer.BeginRenderPass(pass, &pass->targets[frameData.renderTargetIndex]))
    {
        ERROR_LOG("BeginRenderPass failed for pass width id: '{}'.", pass->id);
        return false;
    }

    const auto frameNumber = frameData.frameNumber;
    const auto drawIndex   = frameData.drawIndex;

    WireframeShaderInfo* shaderInfos[2]                                                   = { &m_meshShader, &m_terrainShader };
    const C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator>* geometries[2] = { &packet->geometries,
                                                                                              &packet->terrainGeometries };

    for (u8 i = 0; i < 2; i++)
    {
        auto shaderInfo = shaderInfos[i];

        // Use our shader
        if (!Shaders.UseById(shaderInfo->shader->id))
        {
            ERROR_LOG("Failed to use shader: '{}'.", shaderInfo->shader->name);
            return false;
        }

        // Set global uniforms
        Renderer.BindShaderGlobals(*shaderInfo->shader);

        if (!Shaders.SetUniformByIndex(shaderInfo->locations.projection, &packet->projectionMatrix))
        {
            ERROR_LOG("Failed to set Projection uniform for Wireframe Shader.");
            return false;
        }

        if (!Shaders.SetUniformByIndex(shaderInfo->locations.view, &packet->viewMatrix))
        {
            ERROR_LOG("Failed to set View uniform for Wireframe Shader.");
            return false;
        }

        Shaders.ApplyGlobal(true);

        // Geometries
        for (auto& geometry : *geometries[i])
        {
            // Set instance uniforms

            // Selecting the instance allows us to easily switch colors
            WireframeColorInstance& instance =
                geometry.uniqueId == m_selectedId ? shaderInfo->selectedInstance : shaderInfo->normalInstance;

            // Bind our current instance
            Shaders.BindInstance(instance.id);

            const bool needsUpdate = instance.frameNumber != frameNumber || instance.drawIndex != drawIndex;
            if (needsUpdate)
            {
                if (!Shaders.SetUniformByIndex(shaderInfo->locations.color, &instance.color))
                {
                    ERROR_LOG("Failed to set Color uniform for Wireframe Shader.");
                    return false;
                }
            }

            Shaders.ApplyInstance(needsUpdate);

            // Sync the frame number and draw index with the current
            instance.frameNumber = frameNumber;
            instance.drawIndex   = drawIndex;

            // Set the locals (model)
            if (!Shaders.SetUniformByIndex(shaderInfo->locations.model, &geometry.model))
            {
                ERROR_LOG("Failed to set Model uniform for Wireframe Shader.");
                return false;
            }

            // Actually draw our geometry
            Renderer.DrawGeometry(geometry);
        }
    }

    if (!Renderer.EndRenderPass(pass))
    {
        ERROR_LOG("EndRenderPass failed for pass with id: '{}'.", pass->id);
        return false;
    }

    return true;
}
