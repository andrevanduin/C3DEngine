
#pragma once
#include "defines.h"
#include "input/buttons.h"
#include "renderer/renderer_types.h"

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
        MouseButtonEventContext(i16 button, i16 x, i16 y) : button(button), x(x), y(y) {}

        i16 button;
        i16 x;
        i16 y;
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

    enum FlagBit : u8
    {
        FlagNone    = 0x00,
        FlagVisible = 0x01,
        FlagActive  = 0x02,
        FlagHovered = 0x04,
        FlagPressed = 0x08,
    };

    using Flags = u8;

    enum ComponentType : i8
    {
        ComponentTypeNone = -1,
        ComponentTypePanel,
        ComponentTypeLabel,
        ComponentTypeButton,
        ComponentTypeTextbox,
    };

    /** @brief IDs for all the different atlas types. For fast lookup. */
    enum AtlasID : u8
    {
        AtlasIDPanel = 0,
        AtlasIDButton,
        AtlasIDTextboxNineSlice,
        AtlasIDTextboxCursor,
        AtlasIDTextboxHighlight,
        AtlasIDMax
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
        GeometryRenderData renderData;
        u32 instanceId  = INVALID_ID;
        u64 frameNumber = INVALID_ID_U64;
        u8 drawIndex    = INVALID_ID_U8;
    };

}  // namespace C3D::UI_2D