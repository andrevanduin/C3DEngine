
#include "ui_pass.h"

#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <resources/font.h>
#include <resources/mesh.h>
#include <resources/ui_text.h>
#include <systems/materials/material_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

constexpr const char* INSTANCE_NAME = "UI_PASS";
constexpr const char* SHADER_NAME   = "Shader.Builtin.UI";

UIPass::UIPass() : RendergraphPass() {}

UIPass::UIPass(const C3D::SystemManager* pSystemsManager) : RendergraphPass("UI", pSystemsManager) {}

bool UIPass::Initialize(const C3D::LinearAllocator* frameAllocator)
{
    C3D::RenderPassConfig pass;
    pass.name       = "RenderPass.UI";
    pass.clearColor = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearNone;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig attachment = {};

    attachment.type           = C3D::RenderTargetAttachmentType::Color;
    attachment.source         = C3D::RenderTargetAttachmentSource::Default;
    attachment.loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
    attachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    attachment.presentAfter   = true;

    pass.target.attachments.PushBack(attachment);
    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    m_pass = Renderer.CreateRenderPass(pass);
    if (!m_pass)
    {
        ERROR_LOG("Failed to create RenderPass.");
        return false;
    }

    C3D::ShaderConfig config;
    if (!Resources.Load(SHADER_NAME, config))
    {
        ERROR_LOG("Failed to load ShaderResource for UI Shader.");
        return false;
    }

    if (!Shaders.Create(m_pass, config))
    {
        ERROR_LOG("Failed to load UI Shader.");
        return false;
    }
    Resources.Unload(config);

    m_shader = Shaders.Get(SHADER_NAME);
    if (!m_shader)
    {
        ERROR_LOG("Failed to get the: '{}' Shader.", SHADER_NAME);
        return false;
    }

    m_locations.diffuseMap = m_shader->GetUniformIndex("diffuseTexture");
    m_locations.properties = m_shader->GetUniformIndex("properties");
    m_locations.model      = m_shader->GetUniformIndex("model");

    m_geometries.SetAllocator(frameAllocator);
    m_texts.SetAllocator(frameAllocator);

    return true;
}

bool UIPass::Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::UIMesh* meshes,
                     const C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator>& texts)
{
    m_geometries.Reset();

    m_viewport = viewport;
    m_camera   = camera;

    for (u8 i = 0; i < 10; i++)
    {
        const auto& mesh = meshes[i];
        if (mesh.generation != INVALID_ID_U8)
        {
            const auto world = mesh.transform.GetWorld();
            for (const auto geometry : mesh.geometries)
            {
                m_geometries.EmplaceBack(world, geometry);
            }
        }
    }
    m_texts    = texts;
    m_prepared = true;
    return true;
}

bool UIPass::Execute(const C3D::FrameData& frameData)
{
    Renderer.SetActiveViewport(m_viewport);

    const auto shaderId = m_shader->id;

    if (!Renderer.BeginRenderPass(m_pass, frameData))
    {
        ERROR_LOG("BeginRenderPass failed for pass: '{}'.", m_pass->GetName());
        return false;
    }

    if (!Shaders.UseById(shaderId))
    {
        ERROR_LOG("Failed to use shader with id: {}.", shaderId);
        return false;
    }

    auto viewMatrix = mat4(1.0f);  // m_camera->GetViewMatrix();

    if (!Materials.ApplyGlobal(shaderId, frameData, &m_viewport->GetProjection(), &viewMatrix, nullptr, nullptr, 0))
    {
        ERROR_LOG("Failed to apply globals for shader with id: {}.", shaderId);
        return false;
    }

    for (auto& data : m_geometries)
    {
        C3D::Material* m = data.geometry->material ? data.geometry->material : Materials.GetDefaultUI();

        const bool needsUpdate = m->renderFrameNumber != frameData.frameNumber;
        if (!Materials.ApplyInstance(m, frameData, needsUpdate))
        {
            WARN_LOG("Failed to apply material: '{}'. Skipping draw.", m->name);
            continue;
        }

        // Sync the frame number with the current
        m->renderFrameNumber = frameData.frameNumber;

        Materials.ApplyLocal(m, &data.model);
        Renderer.DrawGeometry(data);
    }

    for (const auto uiText : m_texts)
    {
        Shaders.BindInstance(uiText->instanceId);

        if (!Shaders.SetUniformByIndex(m_locations.diffuseMap, &uiText->data->atlas))
        {
            ERROR_LOG("Failed to apply bitmap font diffuse map uniform.");
            return false;
        }

        // TODO: Font color
        static constexpr auto white_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (!Shaders.SetUniformByIndex(m_locations.properties, &white_color))
        {
            ERROR_LOG("Failed to apply bitmap font color uniform.");
            return false;
        }

        Shaders.ApplyInstance(uiText->frameNumber != frameData.frameNumber || uiText->drawIndex != frameData.drawIndex);
        uiText->frameNumber = frameData.frameNumber;
        uiText->drawIndex   = frameData.drawIndex;

        auto model = uiText->transform.GetWorld();
        if (!Shaders.SetUniformByIndex(m_locations.model, &model))
        {
            ERROR_LOG("Failed to apply model matrix for text.");
            return false;
        }

        uiText->Draw();
    }

    if (!Renderer.EndRenderPass(m_pass))
    {
        ERROR_LOG("EndRenderPass failed for pass: '{}'.", m_pass->GetName());
        return false;
    }

    return true;
}