
#include "render_view.h"

#include "renderer_frontend.h"
#include "core/events/event.h"
#include "core/events/event_callback.h"
#include "systems/render_view_system.h"

namespace C3D
{
	RenderView::RenderView(const u16 _id, const RenderViewConfig& config)
		: id(_id), name(config.name), type(), m_width(config.width), m_height(config.height), m_customShaderName(nullptr), m_logger(config.name.Data())
	{
		if (!Event.Register(SystemEventCode::DefaultRenderTargetRefreshRequired, new EventCallback(this, &RenderView::OnRenderTargetRefreshRequired)))
		{
			m_logger.Fatal("Constructor() - Unable to register for OnRenderTargetRefreshRequired event.");
		}
	}

	void RenderView::OnDestroy()
	{
		Event.UnRegister(SystemEventCode::DefaultRenderTargetRefreshRequired, new EventCallback(this, &RenderView::OnRenderTargetRefreshRequired));
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

	void RenderView::OnDestroyPacket(RenderViewPacket& packet)
	{
		packet.geometries.Clear();
	}

	void RenderView::OnResize() {}

	bool RenderView::RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment)
	{
		return true;
	}

	bool RenderView::OnRenderTargetRefreshRequired(u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::DefaultRenderTargetRefreshRequired)
		{
			Views.RegenerateRenderTargets(this);
		}
		return false;
	}
}
