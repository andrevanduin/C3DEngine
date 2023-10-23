
#include "render_view_editor_world.h"

#include <renderer/renderer_frontend.h>
#include <systems/cameras/camera_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

#include "editor/editor_gizmo.h"

RenderViewEditorWorld::RenderViewEditorWorld() : RenderView("EDITOR_WORLD_VIEW", "") {}

void RenderViewEditorWorld::OnSetupPasses()
{
    C3D::RenderPassConfig pass;
    pass.name       = "RenderPass.TestEnv.EditorWorld";
    pass.renderArea = { 0, 0, 1280, 720 };
    pass.clearColor = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig targetAttachments[2]{};
    // Color
    targetAttachments[0].type           = C3D::RenderTargetAttachmentType::Color;
    targetAttachments[0].source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
    targetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[0].presentAfter   = false;
    // Depth
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

bool RenderViewEditorWorld::OnCreate()
{
    // Builtin skybox shader
    const auto shaderName = "Shader.Builtin.Color3DShader";
    m_shader              = Shaders.Get(shaderName);
    if (!m_shader)
    {
        m_logger.Error("OnCreate() - Failed to get Color3DShader.");
        return false;
    }

    m_debugShaderLocations.projection = m_shader->GetUniformIndex("projection");
    m_debugShaderLocations.view       = m_shader->GetUniformIndex("view");
    m_debugShaderLocations.model      = m_shader->GetUniformIndex("model");

    const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
    m_projectionMatrix     = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);

    m_camera = Cam.GetDefault();
    return true;
}

void RenderViewEditorWorld::OnResize()
{
    const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
    m_projectionMatrix     = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);
}

bool RenderViewEditorWorld::OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket)
{
    if (!data || !outPacket)
    {
        m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket.");
        return false;
    }

    const auto& packetData = *static_cast<EditorWorldPacketData*>(data);

    outPacket->view             = this;
    outPacket->projectionMatrix = m_projectionMatrix;
    outPacket->viewMatrix       = m_camera->GetViewMatrix();

    outPacket->geometries.SetAllocator(frameAllocator);

    if (packetData.gizmo)
    {
        vec3 camPos   = m_camera->GetPosition();
        vec3 gizmoPos = packetData.gizmo->GetPosition();

        // TODO: Should get this from the camera/viewport
        // FOV
        f32 dist = glm::distance(camPos, gizmoPos);

        mat4 model = packetData.gizmo->GetModel();
        // TODO: Make this configurable
        constexpr f32 fixedSize = 0.1f;
        const f32 scaleScalar   = 1.0f;  // ((2.0f * C3D::Tan(m_fov * 0.5f)) * dist) * fixedSize;

        // Keep a copy of the scale for use with hit-detection
        packetData.gizmo->SetScale(scaleScalar);

        mat4 scale = glm::scale(vec3(scaleScalar));
        model *= scale;

        outPacket->geometries.EmplaceBack(model, packetData.gizmo->GetGeometry(), INVALID_ID);
    }

    return true;
}

bool RenderViewEditorWorld::OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                                     u64 renderTargetIndex)
{
    for (const auto pass : m_passes)
    {
        if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
        {
            m_logger.Error("OnRender() - Failed to begin renderpass: '{}'.", pass->GetName());
            return false;
        }

        if (!Shaders.UseById(m_shader->id))
        {
            m_logger.Error("OnRender() - Failed to use shader by id: '{}'.", m_shader->name);
            return false;
        }

        if (!Renderer.ShaderBindGlobals(*m_shader))
        {
            m_logger.Error("OnRender() - Failed bind globals for shader: '{}'.", m_shader->name);
            return false;
        }

        bool needsUpdate = frameNumber != m_shader->frameNumber;
        if (needsUpdate)
        {
            Shaders.SetUniformByIndex(m_debugShaderLocations.projection, &packet->projectionMatrix);
            Shaders.SetUniformByIndex(m_debugShaderLocations.view, &packet->viewMatrix);
        }
        Shaders.ApplyGlobal(needsUpdate);

        for (const auto& data : packet->geometries)
        {
            // NOTE: No instance-level uniforms need to be set
            // Set our model matrix
            Shaders.SetUniformByIndex(m_debugShaderLocations.model, &data.model);

            // Draw it
            Renderer.DrawGeometry(data);
        }

        if (!Renderer.EndRenderPass(pass))
        {
            m_logger.Error("OnRender() - Failed to end renderpass: '{}'.", pass->GetName());
            return false;
        }
    }

    return true;
}