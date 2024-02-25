
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/ecs/entity_id.h"
#include "flags.h"

namespace C3D
{
    namespace UI_2D
    {
        constexpr u32 COMPONENT_COUNT = 3;

        struct TransformComponent
        {
            vec2 position;

            constexpr static ComponentID GetId() { return 0; }
        };

        struct FlagComponent
        {
            Flags flags;

            constexpr static ComponentID GetId() { return 1; }
        };

        struct ParentComponent
        {
            DynamicArray<EntityID> children;

            constexpr static ComponentID GetId() { return 2; }
        };

    }  // namespace UI_2D
}  // namespace C3D