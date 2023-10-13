
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
    class C3D_API Plane3D
    {
    public:
        Plane3D();
        Plane3D(const vec3& p1, const vec3& norm);
        Plane3D(float a, float b, float c, float d);

        [[nodiscard]] f32 SignedDistance(const vec3& position) const;

        [[nodiscard]] bool IntersectsWithSphere(const Sphere& sphere) const;

        [[nodiscard]] bool IntersectsWithAABB(const AABB& aabb) const;

        vec3 normal;
        f32 distance;
    };
}  // namespace C3D