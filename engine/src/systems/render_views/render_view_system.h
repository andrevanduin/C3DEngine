
#pragma once
#include "containers/hash_map.h"
#include "renderer/render_view.h"
#include "systems/system.h"

namespace C3D
{
    struct RenderViewSystemConfig
    {
        u16 maxViewCount;
    };

    class C3D_API RenderViewSystem final : public SystemWithConfig<RenderViewSystemConfig>
    {
    public:
        explicit RenderViewSystem(const SystemManager* pSystemsManager);

        bool OnInit(const RenderViewSystemConfig& config) override;
        void OnShutdown() override;

        bool Register(RenderView* view);

        void OnWindowResize(u32 width, u32 height) const;

        RenderView* Get(const String& name);

        bool BuildPacket(RenderView* view, const FrameData& frameData, const Viewport& viewport, C3D::Camera* camera, void* data,
                         RenderViewPacket* outPacket) const;
        void DestroyPacket(RenderView* view, RenderViewPacket& packet) const;

        bool OnRender(const FrameData& frameData, RenderView* view, const RenderViewPacket* packet, u64 frameNumber,
                      u64 renderTargetIndex) const;

    private:
        HashMap<String, RenderView*> m_registeredViews;
    };
}  // namespace C3D
