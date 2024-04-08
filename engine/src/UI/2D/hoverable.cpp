
#include "hoverable.h"

namespace C3D::UI_2D
{
    bool Hoverable::OnHoverStart(Component& self, const OnHoverEventContext& ctx)
    {
        self.SetFlag(FlagHovered);
        return true;
    }

    bool Hoverable::OnHoverEnd(Component& self, const OnHoverEventContext& ctx)
    {
        self.RemoveFlag(FlagHovered);
        return true;
    }
}  // namespace C3D::UI_2D