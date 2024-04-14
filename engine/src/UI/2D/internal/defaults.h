
#pragma once
#include "UI/2D/component.h"

namespace C3D::UI_2D
{
    namespace DefaultMethods
    {
        bool OnHoverStart(Component& self, const OnHoverEventContext& ctx);
        bool OnHoverEnd(Component& self, const OnHoverEventContext& ctx);

        bool OnClick(Component& self, const MouseButtonEventContext& ctx);
    }  // namespace DefaultMethods
}  // namespace C3D::UI_2D
