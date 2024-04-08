
#include "textbox.h"

#include "nine_slice_component.h"
#include "text_component.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr auto CORNER_SIZE          = u16vec2(1, 1);

    struct TextboxData
    {
        TextComponent textComponent;
        NineSliceComponent nineSlice;
    };

    Component Textbox::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.m_pImplData = pAllocator->New<TextboxData>(MemoryType::UI);
        component.onDestroy   = &Destroy;
        component.onRender    = &OnRender;

        return component;
    }

    bool Textbox::Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const String& text, FontHandle font)
    {
        self.Initialize(pos, size);
        auto& data = self.GetInternal<TextboxData>();
        data.nineSlice.Initialize(self, "TextboxNineSlice", ComponentTypeTextbox, CORNER_SIZE);
        data.textComponent.Initialize(self, font, text);
        return true;
    }

    void Textbox::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<TextboxData>();
        data.nineSlice.OnRender(self, frameData, locations);
        data.textComponent.OnRender(self, frameData, locations);
    }

    void Textbox::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<TextboxData>();
        data.nineSlice.Destroy(self);
        data.textComponent.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D