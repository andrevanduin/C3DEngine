
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "math_types.h"

namespace C3D
{
    /** @brief Represents a line which starts at the origin and proceeds infinitely in the given direction.
     * Typically used for things like hit tests and picking etc.*/
    class C3D_API Ray
    {
    public:
        vec3 origin;
        vec3 direction;

        Ray() = default;

        Ray(const vec3& position, const vec3& direction);

        bool TestAgainstExtents(const Extents3D& extents, const mat4& model, f32& distance) const;

        static Ray FromScreen(const vec2& screenPos, const vec2& viewportSize, const vec3& origin, const mat4& view,
                              const mat4& projection);
    };

    enum RayCastHitType
    {
        RAY_CAST_HIT_TYPE_NONE,
        RAY_CAST_HIT_TYPE_OBB,
        RAY_CAST_HIT_TYPE_SURFACE
    };

    struct RayCastHit
    {
        RayCastHit() = default;

        RayCastHit(RayCastHitType type, u32 uniqueId, vec3 position, f32 distance)
            : type(type), uniqueId(uniqueId), position(position), distance(distance)
        {}

        RayCastHitType type = RAY_CAST_HIT_TYPE_NONE;
        u32 uniqueId        = INVALID_ID;
        vec3 position;
        f32 distance = 0.0f;
    };

    struct RayCastResult
    {
        DynamicArray<RayCastHit> hits;
    };
}  // namespace C3D