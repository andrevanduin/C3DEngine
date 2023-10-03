
#include "core/engine.h"
#include "render_view_ui.h"
#include "renderer/renderer_frontend.h"
#include "resources/font.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "resources/ui_text.h"
#include "systems/materials/material_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    RenderViewUi::RenderViewUi(const RenderViewConfig& config)
        : RenderView(ToUnderlying(RenderViewKnownType::UI), config)
    {}

    bool RenderViewUi::OnCreate()
    {
        // Builtin skybox shader
        const auto shaderName = "Shader.Builtin.UI";
        ShaderConfig shaderConfig;
        if (!Resources.Load(shaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load ShaderResource");
            return false;
        }
        // NOTE: Since this view only has 1 pass we assume index 0
        if (!Shaders.Create(passes[0], shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create {}", shaderName);
            return false;
        }

        Resources.Unload(shaderConfig);

        m_shader             = Shaders.Get(m_customShaderName ? m_customShaderName.Data() : shaderName);
        m_diffuseMapLocation = Shaders.GetUniformIndex(m_shader, "diffuseTexture");
        m_propertiesLocation = Shaders.GetUniformIndex(m_shader, "properties");
        m_modelLocation      = Shaders.GetUniformIndex(m_shader, "model");

        // TODO: Set this from configuration
        m_nearClip = -100.0f;
        m_farClip  = 100.0f;

        const auto fWidth  = static_cast<f32>(m_width);
        const auto fHeight = static_cast<f32>(m_height);

        m_projectionMatrix = glm::ortho(0.0f, fWidth, fHeight, 0.0f, m_nearClip, m_farClip);

        return true;
    }

    void RenderViewUi::OnResize()
    {
        m_projectionMatrix =
            glm::ortho(0.0f, static_cast<f32>(m_width), static_cast<f32>(m_height), 0.0f, m_nearClip, m_farClip);
    }

    bool RenderViewUi::OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket)
    {
        if (!data || !outPacket)
        {
            m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
            return false;
        }

        const auto uiData = static_cast<UIPacketData*>(data);

        outPacket->view             = this;
        outPacket->projectionMatrix = m_projectionMatrix;
        outPacket->viewMatrix       = m_viewMatrix;
        outPacket->extendedData     = frameAllocator->Allocate<UIPacketData>(MemoryType::RenderView);
        *static_cast<UIPacketData*>(outPacket->extendedData) = *uiData;

        for (const auto mesh : uiData->meshData.meshes)
        {
            for (const auto geometry : mesh->geometries)
            {
                outPacket->geometries.EmplaceBack(mesh->transform.GetWorld(), geometry);
            }
        }

        return true;
    }

    bool RenderViewUi::OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                                u64 renderTargetIndex)
    {
        for (const auto pass : passes)
        {
            const auto shaderId = m_shader->id;

            if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
            {
                m_logger.Error("OnRender() - BeginRenderPass failed for pass with id '{}'.", pass->id);
                return false;
            }

            if (!Shaders.UseById(shaderId))
            {
                m_logger.Error("OnRender() - Failed to use shader with id {}.", shaderId);
                return false;
            }

            if (!Materials.ApplyGlobal(shaderId, frameNumber, &packet->projectionMatrix, &packet->viewMatrix, nullptr,
                                       nullptr, 0))
            {
                m_logger.Error("OnRender() - Failed to apply globals for shader with id {}.", shaderId);
                return false;
            }

            for (auto& geometry : packet->geometries)
            {
                Material* m = geometry.geometry->material ? geometry.geometry->material : Materials.GetDefaultUI();

                const bool needsUpdate = m->renderFrameNumber != frameNumber;
                if (!Materials.ApplyInstance(m, needsUpdate))
                {
                    m_logger.Warn("Failed to apply material '{}'. Skipping draw.", m->name);
                    continue;
                }

                // Sync the frame number with the current
                m->renderFrameNumber = static_cast<u32>(frameNumber);

                Materials.ApplyLocal(m, &geometry.model);
                Renderer.DrawGeometry(geometry);
            }

            const auto packetData = static_cast<UIPacketData*>(packet->extendedData);
            for (const auto uiText : packetData->texts)
            {
                Shaders.BindInstance(uiText->instanceId);

                if (!Shaders.SetUniformByIndex(m_diffuseMapLocation, &uiText->data->atlas))
                {
                    m_logger.Error("OnRender() - Failed to apply bitmap font diffuse map uniform.");
                    return false;
                }

                // TODO: Font color
                static constexpr auto white_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
                if (!Shaders.SetUniformByIndex(m_propertiesLocation, &white_color))
                {
                    m_logger.Error("OnRender() - Failed to apply bitmap font color uniform.");
                    return false;
                }

                Shaders.ApplyInstance(uiText->frameNumber != frameNumber);
                uiText->frameNumber = frameNumber;

                auto model = uiText->transform.GetWorld();
                if (!Shaders.SetUniformByIndex(m_modelLocation, &model))
                {
                    m_logger.Error("OnRender() - Failed to apply model matrix for text.");
                    return false;
                }

                uiText->Draw();
            }

            if (!Renderer.EndRenderPass(pass))
            {
                m_logger.Error("OnRender() - EndRenderPass failed for pass with id '{}'", pass->id);
                return false;
            }
        }

        return true;
    }
}  // namespace C3D
