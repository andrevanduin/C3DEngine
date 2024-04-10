
#include "button.h"

#include "defaults.h"
#include "nine_slice_component.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    struct ButtonData
    {
        NineSliceComponent nineSlice;
    };

    Component Button::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.MakeInternal<ButtonData>(pAllocator);
        component.onDestroy = &Destroy;
        component.onRender  = &OnRender;
        component.onClick   = &DefaultMethods::OnClick;

        return component;
    }

    bool Button::Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const u16vec2& cornerSize)
    {
        self.Initialize(pos, size, ComponentTypeButton);
        auto& data = self.GetInternal<ButtonData>();
        data.nineSlice.Initialize(self, "Button", AtlasIDButton, size, cornerSize);
        return true;
    }

    void Button::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<ButtonData>();
        data.nineSlice.OnRender(self, frameData, locations);
    }

    void Button::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<ButtonData>();
        data.nineSlice.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D