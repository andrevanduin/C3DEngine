
#include "skybox_pass.h"

#include <renderer/camera.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <resources/loaders/shader_loader.h>
#include <resources/shaders/shader_types.h>
#include <resources/skybox.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

constexpr const char* INSTANCE_NAME      = "SKYBOX_PASS";
constexpr const char* SKYBOX_SHADER_NAME = "Shader.Builtin.Skybox";

SkyboxPass::SkyboxPass() : Renderpass() {}

SkyboxPass::SkyboxPass(const C3D::SystemManager* pSystemsManager) : Renderpass("SKYBOX", pSystemsManager) {}

bool SkyboxPass::Initialize(const C3D::LinearAllocator* frameAllocator)
{
    C3D::RenderpassConfig pass = {};
    pass.name                  = "Renderpass.Skybox";
    pass.clearColor            = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags            = C3D::ClearColorBuffer;
    pass.depth                 = 1.0f;
    pass.stencil               = 0;

    C3D::RenderTargetAttachmentConfig targetAttachment = {};
    targetAttachment.type                              = C3D::RenderTargetAttachmentTypeColor;
    targetAttachment.source                            = C3D::RenderTargetAttachmentSource::Default;
    targetAttachment.loadOperation                     = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachment.storeOperation                    = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachment.presentAfter                      = false;

    pass.target.attachments.PushBack(targetAttachment);
    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    if (!CreateInternals(pass))
    {
        ERROR_LOG("Failed to Create Renderpass internals");
        return false;
    }

    C3D::ShaderConfig config;
    if (!Resources.Load(SKYBOX_SHADER_NAME, config))
    {
        ERROR_LOG("Failed to load ShaderResource for Skybox Shader.");
        return false;
    }

    if (!Shaders.Create(m_pInternalData, config))
    {
        ERROR_LOG("Failed to load builtin Skybox Shader.");
        return false;
    }
    Resources.Unload(config);

    m_shader = Shaders.Get(SKYBOX_SHADER_NAME);
    if (!m_shader)
    {
        ERROR_LOG("Failed to get the: '{}' Shader.", SKYBOX_SHADER_NAME);
        return false;
    }

    m_locations.view       = m_shader->GetUniformIndex("view");
    m_locations.projection = m_shader->GetUniformIndex("projection");
    m_locations.cubeMap    = m_shader->GetUniformIndex("cubeTexture");

    return true;
}

bool SkyboxPass::Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::Skybox* skybox)
{
    m_viewport = viewport;
    m_camera   = camera;
    m_skybox   = skybox;
    m_prepared = true;
    return true;
}

bool SkyboxPass::Execute(const C3D::FrameData& frameData)
{
    // Bind our viewport
    Renderer.SetActiveViewport(m_viewport);

    Begin(frameData);

    if (m_skybox)
    {
        Shaders.UseById(m_shader->id);

        // Get the view matrix, but zero out the position so the skybox stays put on screen.
        mat4 viewMatrix  = m_camera->GetViewMatrix();
        viewMatrix[3][0] = 0.0f;
        viewMatrix[3][1] = 0.0f;
        viewMatrix[3][2] = 0.0f;

        // Apply our globals
        Renderer.BindShaderGlobals(*m_shader);

        if (!Shaders.SetUniformByIndex(m_locations.projection, &m_viewport->GetProjection()))
        {
            ERROR_LOG("Failed to apply Skybox projection uniform.");
            return false;
        }

        if (!Shaders.SetUniformByIndex(m_locations.view, &viewMatrix))
        {
            ERROR_LOG("Failed to apply Skybox view uniform.");
            return false;
        }
        Shaders.ApplyGlobal(frameData, true);

        // Instance
        Shaders.BindInstance(m_skybox->instanceId);
        if (!Shaders.SetUniformByIndex(m_locations.cubeMap, &m_skybox->cubeMap))
        {
            ERROR_LOG("Failed to apply Skybox cubemap uniform.");
            return false;
        }

        bool needsUpdate = m_skybox->frameNumber != frameData.frameNumber || m_skybox->drawIndex != frameData.drawIndex;
        Shaders.ApplyInstance(frameData, needsUpdate);

        // Sync the frame number and draw index
        m_skybox->frameNumber = frameData.frameNumber;
        m_skybox->drawIndex   = frameData.drawIndex;

        // Draw it
        const auto renderData = C3D::GeometryRenderData(m_skybox->g);
        Renderer.DrawGeometry(renderData);
    }

    End();

    return true;
}
