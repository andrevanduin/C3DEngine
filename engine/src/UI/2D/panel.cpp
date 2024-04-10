
#include "panel.h"

#include "core/colors.h"
#include "nine_slice_component.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    struct PanelData
    {
        NineSliceComponent nineSlice;
    };

    Component Panel::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.MakeInternal<PanelData>(pAllocator);
        component.onDestroy = &Destroy;
        component.onRender  = &OnRender;
        component.onResize  = &OnResize;

        return component;
    }

    bool Panel::Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const u16vec2& cornerSize)
    {
        self.Initialize(pos, size, ComponentTypePanel);
        auto& data = self.GetInternal<PanelData>();
        data.nineSlice.Initialize(self, "Panel", AtlasIDPanel, size, cornerSize);
        return true;
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