
#include "render_view.h"

#include "core/engine.h"
#include "renderer_frontend.h"
#include "systems/events/event_system.h"
#include "systems/render_views/render_view_system.h"

namespace C3D
{
    RenderView::RenderView(const u16 _id, const RenderViewConfig& config)
        : id(_id),
          name(config.name),
          type(),
          m_width(config.width),
          m_height(config.height),
          m_customShaderName(nullptr),
          m_logger(config.name.Data()),
          m_engine(config.engine)
    {
        m_defaultRenderTargetRefreshRequiredCallback =
            Event.Register(EventCodeDefaultRenderTargetRefreshRequired,
                           [this](const u16 code, void* sender, const EventContext& context) {
                               return OnRenderTargetRefreshRequired(code, sender, context);
                           });
    }

    void RenderView::OnDestroy()
    {
        Event.Unregister(m_defaultRenderTargetRefreshRequiredCallback);
        for (const auto pass : passes)
        {
            Renderer.DestroyRenderPass(pass);
        }
    }

    void RenderView::OnBaseResize(const u32 width, const u32 height)
    {
        if (width != m_width || height != m_height)
        {
            m_width = static_cast<u16>(width);
            m_height = static_cast<u16>(height);

            for (const auto pass : passes)
            {
                pass->renderArea = ivec4(0, 0, m_width, m_height);
            }

            OnResize();
        }
    }

    void RenderView::OnDestroyPacket(RenderViewPacket& packet) { packet.geometries.Clear(); }

    void RenderView::OnResize() {}

    bool RenderView::RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment) { return true; }

    bool RenderView::OnRenderTargetRefreshRequired(const u16 code, void*, const EventContext&)
    {
        if (code == EventCodeDefaultRenderTargetRefreshRequired)
        {
            Views.RegenerateRenderTargets(this);
        }
        return false;
    }
}  // namespace C3D
