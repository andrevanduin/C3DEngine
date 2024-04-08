
#pragma once
#include "core/defines.h"
#include "core/input/buttons.h"

namespace C3D::UI_2D
{
    struct ShaderLocations
    {
        u16 projection     = INVALID_ID_U16;
        u16 view           = INVALID_ID_U16;
        u16 diffuseTexture = INVALID_ID_U16;
        u16 properties     = INVALID_ID_U16;
        u16 model          = INVALID_ID_U16;
    };

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

        u16 x;
        u16 y;
    };

    struct KeyEventContext
    {
        KeyEventContext(u16 keyCode) : keyCode(keyCode) {}

        u16 keyCode;
    };

    // using OnMouseDownEventHandler = std::function<bool(const MouseButtonEventContext& ctx)>;
    // using OnMouseUpEventHandler   = std::function<bool(const MouseButtonEventContext& ctx)>;
    using OnClickEventHandler = std::function<bool(const MouseButtonEventContext& ctx)>;

    using OnHoverStartEventHandler = std::function<bool(const OnHoverEventContext& ctx)>;
    using OnHoverEndEventHandler   = std::function<bool(const OnHoverEventContext& ctx)>;

    enum FlagBit : u8
    {
        FlagNone    = 0x00,
        FlagVisible = 0x01,
        FlagActive  = 0x02,
        FlagHovered = 0x04,
        FlagPressed = 0x08,
    };

    using Flags = u8;

    enum ComponentType : u8
    {
        ComponentTypePanel,
        ComponentTypeLabel,
        ComponentTypeButton,
        ComponentTypeTextbox,
    };

    struct AtlasDescriptions
    {
        u16vec2 defaultMin;
        u16vec2 defaultMax;

        u16vec2 activeMin;
        u16vec2 activeMax;

        u16vec2 hoverMin;
        u16vec2 hoverMax;

        u16vec2 size;
        u16vec2 cornerSize;
    };

    /** @brief Describes the internal data needed for a Component that is renderable. */
    struct RenderableComponent
    {
        RenderableComponent() { Logger::Info("RenderableComponent ctor called"); }

        GeometryRenderData renderData;
        u32 instanceId  = INVALID_ID;
        u64 frameNumber = INVALID_ID_U64;
        u8 drawIndex    = INVALID_ID_U8;
    };

}  // namespace C3D::UI_2D