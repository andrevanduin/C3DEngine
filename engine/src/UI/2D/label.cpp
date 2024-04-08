
#include "label.h"

#include "text_component.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    struct LabelData
    {
        TextComponent textComponent;
    };

    Component Label::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.m_pImplData = pAllocator->New<LabelData>(MemoryType::UI);
        component.onDestroy   = &Destroy;
        component.onRender    = &OnRender;

        return component;
    }

    bool Label::Initialize(Component& self, const u16vec2& pos, const String& text, FontHandle font)
    {
        self.Initialize(pos, u16vec2(0, 0));
        auto& data = self.GetInternal<LabelData>();
        data.textComponent.Initialize(self, font, text);
        return true;
    }

    void Label::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<LabelData>();
        data.textComponent.OnRender(self, frameData, locations);
    }

    void Label::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<LabelData>();
        data.textComponent.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D