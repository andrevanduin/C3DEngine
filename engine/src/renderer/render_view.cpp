
#include "render_view.h"

#include "core/engine.h"
#include "renderer_frontend.h"
#include "resources/textures/texture.h"
#include "systems/events/event_system.h"
#include "systems/render_views/render_view_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDER_VIEW";

    RenderView::RenderView(const String& name, const String& customShaderName) : m_name(name), m_customShaderName(customShaderName) {}

    bool RenderView::OnRegister(const SystemManager* pSystemsManager)
    {
        m_pSystemsManager = pSystemsManager;

        // We Register the RenderTargetRefreshRequired event here since this is the first time
        // we actually have access to the SystemsManager
        m_defaultRenderTargetRefreshRequiredCallback =
            Event.Register(EventCodeDefaultRenderTargetRefreshRequired, [this](const u16 code, void* sender, const EventContext& context) {
                return OnRenderTargetRefreshRequired(code, sender, context);
            });

        // Setup our passes so we can start creating them (again called here because we need the SystemsManager)
        OnSetupPasses();

        // Initialize passes for the view
        for (auto& config : m_passConfigs)
        {
            const auto pass = Renderer.CreateRenderPass(config);
            if (!pass)
            {
                ERROR_LOG("RenderPass: '{}' could not be created.", config.name);
                return false;
            }
            m_passes.PushBack(pass);
        }

        // Call the OnCreate method that the user has specified and return it's result
        return OnCreate();
    }

    void RenderView::OnDestroy()
    {
        Event.Unregister(m_defaultRenderTargetRefreshRequiredCallback);
        for (const auto pass : m_passes)
        {
            Renderer.DestroyRenderPass(pass);
        }
    }

    void RenderView::OnBaseResize(const u32 width, const u32 height)
    {
        if (width != m_width || height != m_height)
        {
            m_width  = static_cast<u16>(width);
            m_height = static_cast<u16>(height);
            OnResize();
        }
    }

    void RenderView::OnDestroyPacket(RenderViewPacket& packet)
    {
        packet.geometries.Clear();
        packet.terrainGeometries.Clear();
        packet.debugGeometries.Clear();
    }

    void RenderView::OnResize() {}

    bool RenderView::RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment) { return true; }

    bool RenderView::OnRenderTargetRefreshRequired(const u16 code, void*, const EventContext&)
    {
        if (code == EventCodeDefaultRenderTargetRefreshRequired)
        {
            RegenerateRenderTargets();
        }
        return false;
    }

    void RenderView::RegenerateRenderTargets()
    {
        for (u32 r = 0; r < m_passes.Size(); r++)
        {
            const auto pass = m_passes[r];

            for (u8 i = 0; i < pass->renderTargetCount; i++)
            {
                auto& target = pass->targets[i];
                // Destroy the old target if it exists
                Renderer.DestroyRenderTarget(&target, false);

                for (u32 a = 0; a < target.attachmentCount; a++)
                {
                    auto& attachment = target.attachments[a];
                    if (attachment.source == RenderTargetAttachmentSource::Default)
                    {
                        if (attachment.type == RenderTargetAttachmentType::Color)
                        {
                            attachment.texture = Renderer.GetWindowAttachment(i);
                        }
                        else if (attachment.type == RenderTargetAttachmentType::Depth)
                        {
                            attachment.texture = Renderer.GetDepthAttachment(i);
                        }
                        else
                        {
                            FATAL_LOG("Attachment type: '{}'.", ToUnderlying(attachment.type));
                        }
                    }
                    else if (attachment.source == RenderTargetAttachmentSource::View)
                    {
                        if (!RegenerateAttachmentTarget(r, &attachment))
                        {
                            ERROR_LOG("View failed to regenerate attachment target for attachment type: '{}'.",
                                      ToUnderlying(attachment.type));
                        }
                    }
                }

                // Create the render target
                Renderer.CreateRenderTarget(target.attachmentCount, target.attachments, pass, target.attachments[0].texture->width,
                                            target.attachments[0].texture->height, &pass->targets[i]);
            }
        }
    }

}  // namespace C3D
