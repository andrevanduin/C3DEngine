
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/entity_id.h"
#include "core/uuid.h"
#include "flags.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"

namespace C3D
{
    namespace UI_2D
    {
        constexpr u32 COMPONENT_COUNT = 7;

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
            DynamicArray<EntityID> children;

            constexpr static ComponentID GetId() { return 5; }
        };

        struct NineSliceComponent
        {
            /** @brief The width and height of the entire nine slice in pixels. */
            u16vec2 size;
            /** @brief The width and height of the actual corner in pixels. */
            u16vec2 cornerSize;
            /** @brief The x and y min values in the atlas. */
            u16vec2 atlasMin;
            /** @brief The x and y max values in the atlas. */
            u16vec2 atlasMax;
            /** @brief The size of the atlas. */
            u16vec2 atlasSize;
            /** @brief The width and height of the corner in the atlas. */
            u16vec2 cornerAtlasSize;

            constexpr static ComponentID GetId() { return 6; }
        };

    }  // namespace UI_2D
}  // namespace C3D