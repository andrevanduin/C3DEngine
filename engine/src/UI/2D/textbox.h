
#pragma once
#include "component.h"
#include "nine_slice_component.h"
#include "quad_component.h"
#include "text_component.h"

namespace C3D::UI_2D
{
    namespace Textbox
    {
        struct InternalData
        {
            TextComponent textComponent;
            NineSliceComponent nineSlice;

            bool showCursor = false;
            QuadComponent cursor;
            f64 nextBlink      = 0.0;
            u32 characterIndex = 0;
        };

        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const String& text, FontHandle font);
        void OnUpdate(Component& self);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self);

        void SetText(Component& self, const char* text);

        void CalculateCursorOffset(Component& self);
        void OnTextChanged(Component& self);

        bool OnKeyDown(Component& self, const KeyEventContext& ctx);
        bool OnClick(Component& self, const MouseButtonEventContext& ctx);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Textbox
}  // namespace C3D::UI_2D