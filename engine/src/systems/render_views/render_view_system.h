
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

        bool Init(const RenderViewSystemConfig& config) override;
        void Shutdown() override;

        bool Create(RenderViewConfig& config);

        void OnWindowResize(u32 width, u32 height) const;

        RenderView* Get(const char* name);

        bool BuildPacket(RenderView* view, LinearAllocator* pFrameAllocator, void* data,
                         RenderViewPacket* outPacket) const;
        void DestroyPacket(RenderView* view, RenderViewPacket& packet) const;

        bool OnRender(RenderView* view, const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const;

        void RegenerateRenderTargets(RenderView* view) const;

    private:
        HashMap<String, RenderView*> m_registeredViews;
    };
}  // namespace C3D
