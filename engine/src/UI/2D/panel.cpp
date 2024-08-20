
#include "panel.h"

#include "core/colors.h"
#include "internal/nine_slice_component.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    struct PanelData
    {
        NineSliceComponent nineSlice;
    };

    Component Panel::Create(const DynamicAllocator* pAllocator)
    {
        Component component;

        component.MakeInternal<PanelData>(pAllocator);
        component.onInitialize    = &Initialize;
        component.onDestroy       = &Destroy;
        component.onPrepareRender = &OnPrepareRender;
        component.onRender        = &OnRender;
        component.onResize        = &OnResize;

        return component;
    }

    bool Panel::Initialize(Component& self, const Config& config)
    {
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.Initialize(self, "Panel", AtlasIDPanel, config.size, config.cornerSize, config.backgroundColor);
        return true;
    }

    void Panel::OnPrepareRender(Component& self)
    {
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.OnPrepareRender(self);
    }

    void Panel::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.OnRender(self, frameData, locations);
    }

    void Panel::OnResize(Component& self)
    {
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.OnResize(self, self.GetSize());
    }

    void Panel::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D