
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    namespace Hoverable
    {
        bool OnHoverStart(Component& self, const OnHoverEventContext& ctx);
        bool OnHoverEnd(Component& self, const OnHoverEventContext& ctx);
    }  // namespace Hoverable
}  // namespace C3D::UI_2D