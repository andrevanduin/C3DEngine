
#include "editor_pass.h"

#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

#include "editor/editor_gizmo.h"

constexpr const char* INSTANCE_NAME = "EDITOR_PASS";
constexpr const char* SHADER_NAME   = "Shader.Builtin.Color3D";

EditorPass::EditorPass() : Renderpass("EDITOR") {}

bool EditorPass::Initialize(const C3D::LinearAllocator* frameAllocator)
{
    C3D::RenderpassConfig pass = {};
    pass.name                  = "Renderpass.Editor";
    pass.clearColor            = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags            = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
    pass.depth                 = 1.0f;
    pass.stencil               = 0;

    C3D::RenderTargetAttachmentConfig targetAttachments[2]{};
    // Color
    targetAttachments[0].type           = C3D::RenderTargetAttachmentTypeColor;
    targetAttachments[0].source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
    targetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[0].presentAfter   = false;
    // Depth
    targetAttachments[1].type           = C3D::RenderTargetAttachmentTypeDepth;
    targetAttachments[1].source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachments[1].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[1].presentAfter   = false;

    pass.target.attachments.PushBack(targetAttachments[0]);
    pass.target.attachments.PushBack(targetAttachments[1]);
    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    if (!CreateInternals(pass))
    {
        ERROR_LOG("Failed to create Renderpass internals.");
        return false;
    }

    m_shader = Shaders.Get(SHADER_NAME);
    if (!m_shader)
    {
        ERROR_LOG("Failed to get the: '{}' Shader.", SHADER_NAME);
        return false;
    }

    m_locations.view       = m_shader->GetUniformIndex("view");
    m_locations.projection = m_shader->GetUniformIndex("projection");
    m_locations.model      = m_shader->GetUniformIndex("model");

    m_geometries.SetAllocator(frameAllocator);

    return true;
}

bool EditorPass::Prepare(C3D::Viewport* viewport, C3D::Camera* camera, EditorGizmo* gizmo)
{
    m_geometries.Reset();

    m_viewport = viewport;
    m_camera   = camera;

    if (gizmo)
    {
        vec3 camPos   = camera->GetPosition();
        vec3 gizmoPos = gizmo->GetPosition();

        // TODO: Should get this from the camera/viewport
        // FOV
        f32 dist = glm::distance(camPos, gizmoPos);

        mat4 model = gizmo->GetModel();
        // TODO: Make this configurable
        constexpr f32 fixedSize = 0.1f;
        const f32 scaleScalar   = 1.0f;  // ((2.0f * C3D::Tan(m_fov * 0.5f)) * dist) * fixedSize;

        // Keep a copy of the scale for use with hit-detection
        gizmo->SetScale(scaleScalar);

        mat4 scale = glm::scale(vec3(scaleScalar));
        model *= scale;

        m_geometries.EmplaceBack(gizmo->GetId(), model, gizmo->GetGeometry());
    }

    m_prepared = true;
    return true;
}

bool EditorPass::Execute(const C3D::FrameData& frameData)
{
    // Bind the viewport
    Renderer.SetActiveViewport(m_viewport);

    Begin(frameData);

    if (!Shaders.UseById(m_shader->id))
    {
        ERROR_LOG("Failed to use shader by id: '{}'.", m_shader->name);
        return false;
    }

    if (!Renderer.BindShaderGlobals(*m_shader))
    {
        ERROR_LOG("Failed bind globals for shader: '{}'.", m_shader->name);
        return false;
    }

    bool needsUpdate = frameData.frameNumber != m_shader->frameNumber || frameData.drawIndex != m_shader->drawIndex;
    if (needsUpdate)
    {
        auto viewMatrix = m_camera->GetViewMatrix();

        Shaders.SetUniformByIndex(m_locations.projection, &m_viewport->GetProjection());
        Shaders.SetUniformByIndex(m_locations.view, &viewMatrix);
    }
    Shaders.ApplyGlobal(frameData, needsUpdate);

    // Sync the frame number and draw index
    m_shader->frameNumber = frameData.frameNumber;
    m_shader->drawIndex   = frameData.drawIndex;

    for (const auto& data : m_geometries)
    {
        // NOTE: No instance-level uniforms need to be set
        // Set our model matrix
        Shaders.BindLocal();
        Shaders.SetUniformByIndex(m_locations.model, &data.model);
        Shaders.ApplyLocal(frameData);

        // Draw it
        Renderer.DrawGeometry(data);
    }

    End();

    return true;
}