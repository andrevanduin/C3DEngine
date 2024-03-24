
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/entity.h"
#include "core/uuid.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "ui2d_defines.h"

namespace C3D::UI_2D
{
    constexpr u32 COMPONENT_COUNT = 9;

    struct IDComponent
    {
        UUID id;

        constexpr static ComponentID GetId() { return 0; }
    };

    struct NameComponent
    {
        String name;

        constexpr static ComponentID GetId() { return 1; }
    };

    struct TransformComponent
    {
        Transform transform;
        u16 width  = 0;
        u16 height = 0;

        constexpr static ComponentID GetId() { return 2; }
    };

    struct FlagsComponent
    {
        Flags flags;

        constexpr static ComponentID GetId() { return 3; }
    };

    struct RenderableComponent
    {
        u32 instanceId  = INVALID_ID;
        u64 frameNumber = INVALID_ID_U64;
        u8 drawIndex    = INVALID_ID_U8;

        Geometry* geometry = nullptr;
        vec4 diffuseColor  = vec4(1.0f);
        u16 depth          = 0;

        constexpr static ComponentID GetId() { return 4; }
    };

    struct ParentComponent
    {
        DynamicArray<Entity> children;

        constexpr static ComponentID GetId() { return 5; }
    };

    struct NineSliceComponent
    {
        /** @brief The width and height of the actual corner in pixels. */
        u16vec2 cornerSize;
        /** @brief The x and y min values in the atlas. */
        u16vec2 atlasMin, atlasMinDefault, atlasMinHover;
        /** @brief The x and y max values in the atlas. */
        u16vec2 atlasMax, atlasMaxDefault, altlasMaxHover;
        /** @brief The x and y max values in the atlas on hover. */
        u16vec2 atlasMaxHover;
        /** @brief The size of the atlas. */
        u16vec2 atlasSize;
        /** @brief The width and height of the corner in the atlas. */
        u16vec2 cornerAtlasSize;

        constexpr static ComponentID GetId() { return 6; }
    };

    struct ClickableComponent
    {
        Bounds bounds;
        OnClickEventHandler onClick;
        OnMouseDownEventHandler onMouseDown;
        OnMouseUpEventHandler onMouseUp;

        constexpr static ComponentID GetId() { return 7; }
    };

    constexpr auto jan = sizeof(OnClickEventHandler);

    struct HoverableComponent
    {
        Bounds bounds;
        OnHoverStartEventHandler onHoverStart;
        OnHoverEndEventHandler onHoverEnd;

        constexpr static ComponentID GetId() { return 8; }
    };
}  // namespace C3D::UI_2D