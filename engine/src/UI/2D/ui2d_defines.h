
#pragma once
#include <functional>

#include "core/defines.h"
#include "core/input/buttons.h"

namespace C3D::UI_2D
{
    struct MouseButtonEventContext
    {
        MouseButtonEventContext(u8 button, u16 x, u16 y) : button(button), x(x), y(y) {}

        u8 button;
        u16 x;
        u16 y;
    };

    struct OnHoverEventContext
    {
        OnHoverEventContext(u16 x, u16 y) : x(x), y(y) {}

        u16 x, y;
    };

    using OnMouseDownEventHandler = std::function<bool(const MouseButtonEventContext& ctx)>;
    using OnMouseUpEventHandler   = std::function<bool(const MouseButtonEventContext& ctx)>;
    using OnClickEventHandler     = std::function<bool(const MouseButtonEventContext& ctx)>;

    using OnHoverStartEventHandler = std::function<bool(const OnHoverEventContext& ctx)>;
    using OnHoverEndEventHandler   = std::function<bool(const OnHoverEventContext& ctx)>;

    enum FlagBit : u8
    {
        FlagNone    = 0x0,
        FlagVisible = 0x1,
        FlagActive  = 0x2,
        FlagHovered = 0x4,
        FlagPressed = 0x8,
    };

    using Flags = u32;
}  // namespace C3D::UI_2D