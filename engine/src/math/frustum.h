
#pragma once
#include "plane.h"

namespace C3D
{
    class Viewport;

    struct C3D_API Frustum
    {
        Frustum() = default;
        Frustum(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, Viewport* viewport);

        void Create(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, Viewport* viewport);

        [[nodiscard]] bool IntersectsWithSphere(const Sphere& sphere) const;

        [[nodiscard]] bool IntersectsWithAABB(const AABB& aabb) const;

        Plane3D sides[6] = {};
    };
}  // namespace C3D