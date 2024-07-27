
#include "button.h"

#include "internal/defaults.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    Component Button::Create(const DynamicAllocator* pAllocator)
    {
        Component component;

        component.MakeInternal<InternalData>(pAllocator);
        component.onInitialize = &Initialize;
        component.onDestroy    = &Destroy;
        component.onRender     = &OnRender;
        component.onClick      = &DefaultMethods::OnClick;

        return component;
    }

    bool Button::Initialize(Component& self, const Config& config)
    {
        auto& data = self.GetInternal<InternalData>();
        data.nineSlice.Initialize(self, "Button", AtlasIDButton, config.size, config.cornerSize);
        return true;
    }

    void Button::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<InternalData>();
        data.nineSlice.OnRender(self, frameData, locations);
    }

    void Button::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<InternalData>();
        data.nineSlice.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D