
#include "ui_pass.h"

#include "UI/2D/component.h"
#include "renderer/camera.h"
#include "renderer/geometry.h"
#include "renderer/renderer_frontend.h"
#include "renderer/rendergraph/renderpass.h"
#include "renderer/viewport.h"
#include "resources/font.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "resources/textures/texture_map.h"
#include "systems/fonts/font_system.h"
#include "systems/materials/material_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"

constexpr const char* INSTANCE_NAME = "UI_PASS";
constexpr const char* SHADER_NAME   = "Shader.Builtin.UI2D";

namespace C3D
{

    UI2DPass::UI2DPass() : Renderpass() {}

    UI2DPass::UI2DPass(const C3D::SystemManager* pSystemsManager) : Renderpass("UI", pSystemsManager) {}

    bool UI2DPass::Initialize(const C3D::LinearAllocator* frameAllocator)
    {
        C3D::RenderpassConfig pass;
        pass.name       = "RenderPass.UI";
        pass.clearColor = { 0, 0, 0.2f, 1.0f };
        pass.clearFlags = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
        pass.depth      = 1.0f;
        pass.stencil    = 0;

        C3D::RenderTargetAttachmentConfig attachments[2] = {};

        attachments[0].type           = C3D::RenderTargetAttachmentTypeColor;
        attachments[0].source         = C3D::RenderTargetAttachmentSource::Default;
        attachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
        attachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
        attachments[0].presentAfter   = true;

        attachments[1].type           = C3D::RenderTargetAttachmentTypeDepth | C3D::RenderTargetAttachmentTypeStencil;
        attachments[1].source         = C3D::RenderTargetAttachmentSource::Default;
        attachments[1].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
        attachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
        attachments[1].presentAfter   = false;

        pass.target.attachments.PushBack(attachments[0]);
        pass.target.attachments.PushBack(attachments[1]);

        pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

        if (!CreateInternals(pass))
        {
            ERROR_LOG("Failed to create Renderpass internals.");
            return false;
        }

        C3D::ShaderConfig config;
        if (!Resources.Load(SHADER_NAME, config))
        {
            ERROR_LOG("Failed to load ShaderResource for UI Shader.");
            return false;
        }

        if (!Shaders.Create(m_pInternalData, config))
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

        m_locations.projection     = m_shader->GetUniformIndex("projection");
        m_locations.view           = m_shader->GetUniformIndex("view");
        m_locations.properties     = m_shader->GetUniformIndex("properties");
        m_locations.diffuseTexture = m_shader->GetUniformIndex("diffuseTexture");
        m_locations.model          = m_shader->GetUniformIndex("model");

        return true;
    }

    void UI2DPass::Prepare(Viewport* viewport, const DynamicArray<UI_2D::Component>* components)
    {
        m_pComponents = components;
        m_viewport    = viewport;
        m_prepared    = true;
    }

    bool UI2DPass::Execute(const C3D::FrameData& frameData)
    {
        Renderer.SetActiveViewport(m_viewport);

        Renderer.SetDepthTestingEnabled(false);

        const auto shaderId = m_shader->id;

        auto viewMatrix = mat4(1.0f);  // m_camera->GetViewMatrix();
        auto projection = &m_viewport->GetProjection();

        Begin(frameData);

        // UI System geometries
        if (!Shaders.UseById(shaderId))
        {
            ERROR_LOG("Failed to use shader with id: {}.", shaderId);
            return false;
        }

        // Apply globals
        Shaders.SetUniformByIndex(m_locations.projection, projection);
        Shaders.SetUniformByIndex(m_locations.view, &viewMatrix);
        Shaders.ApplyGlobal(frameData, true);

        // Sync our frame number
        m_shader->frameNumber = frameData.frameNumber;
        m_shader->drawIndex   = frameData.drawIndex;

        for (auto& component : *m_pComponents)
        {
            if (component.IsFlagSet(UI_2D::FlagVisible))
            {
                component.onRender(component, frameData, m_locations);
            }
        }

        End();
        return true;
    }
}  // namespace C3D