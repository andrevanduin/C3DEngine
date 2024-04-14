
#pragma once
#include "component.h"
#include "config.h"
#include "internal/clipping_component.h"
#include "internal/nine_slice_component.h"
#include "internal/quad_component.h"
#include "internal/text_component.h"

namespace C3D::UI_2D
{
    namespace Textbox
    {
        enum FlagBits : u8
        {
            FlagNone      = 0x0,
            FlagCursor    = 0x1,
            FlagHighlight = 0x2
        };

        using Flags = u8;

        struct InternalData
        {
            TextComponent textComponent;
            NineSliceComponent nineSlice;
            ClippingComponent clip;

            QuadComponent cursor;
            QuadComponent highlight;

            Flags flags = FlagNone;

            f64 nextBlink = 0.0;

            u16 characterIndexStart   = 0;
            u16 characterIndexCurrent = 0;
            u16 characterIndexEnd     = 0;
        };

        Component Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator);

        bool Initialize(Component& self, const Config& config);
        void OnUpdate(Component& self);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self);

        void SetText(Component& self, const char* text);

        void CalculateCursorOffset(Component& self);
        void CalculateHighlight(Component& self, bool shiftDown);

        void OnTextChanged(Component& self);

        bool OnKeyDown(Component& self, const KeyEventContext& ctx);
        bool OnClick(Component& self, const MouseButtonEventContext& ctx);

        void Destroy(Component& self, const DynamicAllocator* pAllocator);

    }  // namespace Textbox
}  // namespace C3D::UI_2D