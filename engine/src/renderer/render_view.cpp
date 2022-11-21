
#include "render_view.h"

#include "core/events/event.h"
#include "core/events/event_callback.h"
#include "systems/render_view_system.h"

namespace C3D
{
	RenderView::RenderView(const u16 id, const RenderViewConfig& config)
		: id(id), name(nullptr), type(), m_width(config.width), m_height(config.height), m_customShaderName(nullptr), m_logger(config.name)
	{
		name = StringDuplicate(config.name);

		if (!Event.Register(SystemEventCode::DefaultRenderTargetRefreshRequired, new EventCallback(this, &RenderView::OnRenderTargetRefreshRequired)))
		{
			m_logger.Fatal("Constructor() - Unable to register for OnRenderTargetRefreshRequired event.");
		}
	}

	void RenderView::OnDestroy()
	{
		const auto length = StringLength(name) + 1;
		Memory.Free(name, length, MemoryType::String);
		Event.UnRegister(SystemEventCode::DefaultRenderTargetRefreshRequired, new EventCallback(this, &RenderView::OnRenderTargetRefreshRequired));
	}

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
