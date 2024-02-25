
#pragma once
#include "core/defines.h"

namespace C3D::UI_2D
{
    enum FlagBit : u8
    {
        FlagNone    = 0x0,
        FlagVisible = 0x1,
        FlagActive  = 0x2,
    };

    using Flags = u32;
}  // namespace C3D::UI_2D