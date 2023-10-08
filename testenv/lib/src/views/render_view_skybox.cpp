
#include "render_view_skybox.h"

#include <core/engine.h>
#include <math/c3d_math.h>
#include <renderer/renderer_frontend.h>
#include <resources/loaders/shader_loader.h>
#include <systems/cameras/camera_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

RenderViewSkybox::RenderViewSkybox() : RenderView("SKYBOX_VIEW", "") {}

void RenderViewSkybox::OnSetupPasses()
{
    C3D::RenderPassConfig pass{};
    pass.name       = "RenderPass.Builtin.Skybox";
    pass.renderArea = { 0, 0, 1280, 720 };
    pass.clearColor = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearColorBuffer;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig targetAttachment = {};

    targetAttachment.type           = C3D::RenderTargetAttachmentType::Color;
    targetAttachment.source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachment.loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachment.presentAfter   = false;

    pass.target.attachments.PushBack(targetAttachment);
    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    m_passConfigs.PushBack(pass);
}

bool RenderViewSkybox::OnCreate()
{
    // Builtin skybox shader
    const auto shaderName = "Shader.Builtin.Skybox";
    C3D::ShaderConfig shaderConfig;
    if (!Resources.Load(shaderName, shaderConfig))
    {
        m_logger.Error("OnCreate() - Failed to load ShaderResource");
        return false;
    }
    // NOTE: Since this view only has 1 pass we assume index 0
    if (!Shaders.Create(m_passes[0], shaderConfig))
    {
        m_logger.Error("OnCreate() - Failed to create {}", shaderName);
        return false;
    }

    Resources.Unload(shaderConfig);

    m_shader             = Shaders.Get(m_customShaderName ? m_customShaderName.Data() : shaderName);
    m_projectionLocation = Shaders.GetUniformIndex(m_shader, "projection");
    m_viewLocation       = Shaders.GetUniformIndex(m_shader, "view");
    m_cubeMapLocation    = Shaders.GetUniformIndex(m_shader, "cubeTexture");

    const auto fWidth  = static_cast<f32>(m_width);
    const auto fHeight = static_cast<f32>(m_height);

    m_projectionMatrix = glm::perspective(m_fov, fWidth / fHeight, m_nearClip, m_farClip);
    m_camera           = Cam.GetDefault();

    return true;
}

void RenderViewSkybox::OnResize()
{
    const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
    m_projectionMatrix     = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);
}

bool RenderViewSkybox::OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket)
{
    if (!data || !outPacket)
    {
        m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
        return false;
    }

    const auto skyboxData = static_cast<C3D::SkyboxPacketData*>(data);

    outPacket->view             = this;
    outPacket->projectionMatrix = m_projectionMatrix;
    outPacket->viewMatrix       = m_camera->GetViewMatrix();
    outPacket->viewPosition     = m_camera->GetPosition();
    outPacket->extendedData     = frameAllocator->Allocate<C3D::SkyboxPacketData>(C3D::MemoryType::RenderView);
    *static_cast<C3D::SkyboxPacketData*>(outPacket->extendedData) = *skyboxData;

    return true;
}

bool RenderViewSkybox::OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                                u64 renderTargetIndex)
{
    const auto skyBoxData = static_cast<C3D::SkyboxPacketData*>(packet->extendedData);

    for (const auto pass : m_passes)
    {
        const auto shaderId = m_shader->id;

        if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
        {
            m_logger.Error("OnRender() - BeginRenderPass failed for pass width id '{}'", pass->id);
            return false;
        }

        if (skyBoxData)
        {
            if (!Shaders.UseById(shaderId))
            {
                m_logger.Error("OnRender() - Failed to use shader with id {}", shaderId);
                return false;
            }

            // Get the view matrix, but zero out the position so the skybox stays in the same spot
            auto view  = m_camera->GetViewMatrix();
            view[3][0] = 0.0f;
            view[3][1] = 0.0f;
            view[3][2] = 0.0f;

            // TODO: Change this

            // Global
            Renderer.ShaderBindGlobals(Shaders.GetById(shaderId));
            if (!Shaders.SetUniformByIndex(m_projectionLocation, &packet->projectionMatrix))
            {
                m_logger.Error("Failed to apply projection matrix.");
                return false;
            }
            if (!Shaders.SetUniformByIndex(m_viewLocation, &view))
            {
                m_logger.Error("Failed to apply view position.");
                return false;
            }
            Shaders.ApplyGlobal();

            // Instance
            Shaders.BindInstance(skyBoxData->box->instanceId);
            if (!Shaders.SetUniformByIndex(m_cubeMapLocation, &skyBoxData->box->cubeMap))
            {
                m_logger.Error("Failed to apply cube map uniform.");
                return false;
            }
            bool needsUpdate = skyBoxData->box->frameNumber != frameNumber;
            Shaders.ApplyInstance(needsUpdate);

            // Sync the frame number
            skyBoxData->box->frameNumber = frameNumber;

            // Draw it
            C3D::GeometryRenderData data(skyBoxData->box->g);
            Renderer.DrawGeometry(data);
        }

        if (!Renderer.EndRenderPass(pass))
        {
            m_logger.Error("OnRender() - EndRenderPass failed for pass with id '{}'", pass->id);
            return false;
        }
    }

    return true;
}