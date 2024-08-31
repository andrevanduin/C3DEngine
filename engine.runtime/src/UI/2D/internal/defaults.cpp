
#include "defaults.h"

#include "systems/UI/2D/ui2d_system.h"

namespace C3D::UI_2D
{
    bool DefaultMethods::OnHoverStart(Component& self, const OnHoverEventContext& ctx)
    {
        self.SetFlag(FlagHovered);
        if (self.pUserHandlers && self.pUserHandlers->onClickHandler)
        {
            return self.pUserHandlers->onHoverStartHandler(ctx);
        }
        return true;
    }

    bool DefaultMethods::OnHoverEnd(Component& self, const OnHoverEventContext& ctx)
    {
        self.RemoveFlag(FlagHovered);
        if (self.pUserHandlers && self.pUserHandlers->onClickHandler)
        {
            return self.pUserHandlers->onHoverEndHandler(ctx);
        }
        return true;
    }

    bool DefaultMethods::OnClick(Component& self, const MouseButtonEventContext& ctx)
    {
        UI2D.SetActive(self, true);

        if (self.pUserHandlers && self.pUserHandlers->onClickHandler)
        {
            return self.pUserHandlers->onClickHandler(ctx);
        }
        return true;
    }
}  // namespace C3D::UI_2D