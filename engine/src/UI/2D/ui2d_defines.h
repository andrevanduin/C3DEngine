
#pragma once
#include <functional>

#include "core/defines.h"
#include "core/input/buttons.h"

namespace C3D::UI_2D
{
    struct OnClickEventContext
    {
        OnClickEventContext(u8 button, u16 x, u16 y) : button(button), x(x), y(y) {}

        u8 button;
        u16 x;
        u16 y;
    };

    using OnClickEventHandler = std::function<bool(const OnClickEventContext& ctx)>;

    enum FlagBit : u8
    {
        FlagNone    = 0x0,
        FlagVisible = 0x1,
        FlagActive  = 0x2,
    };

    using Flags = u32;
}