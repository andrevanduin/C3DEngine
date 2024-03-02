
#include "ui_pass.h"

#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/renderpass.h>
#include <renderer/viewport.h>
#include <resources/font.h>
#include <resources/mesh.h>
#include <resources/textures/texture_map.h>
#include <resources/ui_text.h>
#include <systems/materials/material_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

constexpr const char* INSTANCE_NAME    = "UI_PASS";
constexpr const char* SHADER_NAME      = "Shader.Builtin.UI";
constexpr const char* SHADER_UI2D_NAME = "Shader.Builtin.UI2D";

namespace C3D
{

    UI2DPass::UI2DPass() : RendergraphPass() {}

    UI2DPass::UI2DPass(const C3D::SystemManager* pSystemsManager) : RendergraphPass("UI", pSystemsManager) {}

    bool UI2DPass::Initialize(const C3D::LinearAllocator* frameAllocator)
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

        if (!Resources.Load(SHADER_UI2D_NAME, config))
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

        m_ui2DShader = Shaders.Get(SHADER_UI2D_NAME);
        if (!m_shader)
        {
            ERROR_LOG("Failed to get the: '{}' Shader.", SHADER_NAME);
            return false;
        }

        m_ui2DLocations.projection     = m_ui2DShader->GetUniformIndex("projection");
        m_ui2DLocations.view           = m_ui2DShader->GetUniformIndex("view");
        m_ui2DLocations.properties     = m_ui2DShader->GetUniformIndex("properties");
        m_ui2DLocations.diffuseTexture = m_ui2DShader->GetUniformIndex("diffuseTexture");
        m_ui2DLocations.model          = m_ui2DShader->GetUniformIndex("model");

        return true;
    }

    bool UI2DPass::Prepare(Viewport* viewport, Camera* camera, TextureMap* textureAtlas, const UIMesh* meshes,
                           const DynamicArray<UIText*, LinearAllocator>* texts,
                           const DynamicArray<UIRenderData, LinearAllocator>* uiRenderData)
    {
        m_viewport      = viewport;
        m_camera        = camera;
        m_pMeshes       = meshes;
        m_pTexts        = texts;
        m_pUIRenderData = uiRenderData;
        m_pTextureAtlas = textureAtlas;
        m_prepared      = true;
        return true;
    }

    bool UI2DPass::Execute(const C3D::FrameData& frameData)
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
        auto projection = &m_viewport->GetProjection();

        if (!Materials.ApplyGlobal(shaderId, frameData, projection, &viewMatrix, nullptr, nullptr, 0))
        {
            ERROR_LOG("Failed to apply globals for shader with id: {}.", shaderId);
            return false;
        }

        for (u32 i = 0; i < 10; i++)
        {
            const auto& uiMesh = m_pMeshes[i];
            if (uiMesh.generation == INVALID_ID_U8) continue;

            for (const auto geometry : uiMesh.geometries)
            {
                C3D::Material* m       = geometry->material ? geometry->material : Materials.GetDefaultUI();
                const bool needsUpdate = m->renderFrameNumber != frameData.frameNumber;

                if (!Materials.ApplyInstance(m, frameData, needsUpdate))
                {
                    WARN_LOG("Failed to apply material: '{}'. Skipping draw.", m->name);
                    continue;
                }

                // Sync the frame number with the current
                m->renderFrameNumber = frameData.frameNumber;

                const auto model = uiMesh.transform.GetWorld();
                Materials.ApplyLocal(m, &model);

                GeometryRenderData data(model, geometry, uiMesh.uuid);
                Renderer.DrawGeometry(data);
            }
        }

        for (const auto uiText : *m_pTexts)
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

        // UI System geometries
        if (!Shaders.UseById(m_ui2DShader->id))
        {
            ERROR_LOG("Failed to use shader with id: {}.", shaderId);
            return false;
        }

        // Apply globals
        Shaders.SetUniformByIndex(m_ui2DLocations.projection, projection);
        Shaders.SetUniformByIndex(m_ui2DLocations.view, &viewMatrix);
        Shaders.ApplyGlobal(true);

        // Sync our frame number
        m_ui2DShader->frameNumber = frameData.frameNumber;

        for (auto& data : *m_pUIRenderData)
        {
            // Apply instance
            bool needsUpdate = *data.pFrameNumber != frameData.frameNumber || *data.pDrawIndex != frameData.drawIndex;

            Shaders.BindInstance(data.instanceId);

            Shaders.SetUniformByIndex(m_ui2DLocations.properties, &data.properties);
            Shaders.SetUniformByIndex(m_ui2DLocations.diffuseTexture, m_pTextureAtlas);
            Shaders.ApplyInstance(needsUpdate);

            // Sync frame number
            *data.pFrameNumber = frameData.frameNumber;
            *data.pDrawIndex   = frameData.drawIndex;

            // Apply local
            Shaders.SetUniformByIndex(m_ui2DLocations.model, &data.geometryData.model);

            Renderer.DrawGeometry(data.geometryData);
        }

        if (!Renderer.EndRenderPass(m_pass))
        {
            ERROR_LOG("EndRenderPass failed for pass: '{}'.", m_pass->GetName());
            return false;
        }

        return true;
    }
}  // namespace C3D