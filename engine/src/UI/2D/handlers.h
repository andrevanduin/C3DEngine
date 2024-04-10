
#pragma once
#include <functional>

#include "ui2d_defines.h"

namespace C3D::UI_2D
{
    // using OnMouseDownEventHandler = std::function<bool(const MouseButtonEventContext& ctx)>;
    // using OnMouseUpEventHandler   = std::function<bool(const MouseButtonEventContext& ctx)>;
    using OnClickEventHandler = std::function<bool(const MouseButtonEventContext& ctx)>;

    using OnHoverStartEventHandler = std::function<bool(const OnHoverEventContext& ctx)>;
    using OnHoverEndEventHandler   = std::function<bool(const OnHoverEventContext& ctx)>;

    using OnTextChangedEventHandler = std::function<void(const String& text)>;
    using OnEndTextInputEventHandler = std::function<void(u16 key, const String& text)>;

    /** @brief Handlers that can be added by the user in order for the component to do custom logic.*/
    struct UserHandlers
    {
        OnClickEventHandler onClickHandler;

        OnHoverStartEventHandler onHoverStartHandler;
        OnHoverEndEventHandler onHoverEndHandler;

        OnTextChangedEventHandler onTextChangedHandler;
        OnEndTextInputEventHandler onTextInputEndHandler;
    };
}  // namespace C3D::UI_2D