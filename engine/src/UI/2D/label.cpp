
#include "label.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    Component Label::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.MakeInternal<InternalData>(pAllocator);
        component.onDestroy = &Destroy;
        component.onRender  = &OnRender;

        return component;
    }

    bool Label::Initialize(Component& self, const u16vec2& pos, const String& text, FontHandle font)
    {
        self.Initialize(pos, u16vec2(0, 0), ComponentTypeLabel);
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.Initialize(self, font, text);
        return true;
    }

    void Label::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.OnRender(self, frameData, locations);
    }

    void Label::SetText(Component& self, const String& text)
    {
        auto& data              = self.GetInternal<InternalData>();
        data.textComponent.text = text;
    }

    void Label::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D