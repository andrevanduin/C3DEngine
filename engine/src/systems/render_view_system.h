
#pragma once
#include "system.h"
#include "containers/hash_map.h"
#include "containers/hash_table.h"

#include "renderer/render_view.h"

namespace C3D
{
	struct RenderViewSystemConfig
	{
		u16 maxViewCount;
	};

	class C3D_API RenderViewSystem final : public System<RenderViewSystemConfig>
	{
	public:
		RenderViewSystem();

		bool Init(const RenderViewSystemConfig& config) override;
		void Shutdown() override;

		bool Create(const RenderViewConfig& config);

		void OnWindowResize(u32 width, u32 height) const;

		RenderView* Get(const char* name);

		bool BuildPacket(RenderView* view, void* data, RenderViewPacket* outPacket) const;
		bool OnRender(RenderView* view, const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const;

		void RegenerateRenderTargets(RenderView* view) const;

	private:
		HashMap<String, RenderView*> m_registeredViews;
	};
}
