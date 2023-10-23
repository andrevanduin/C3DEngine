
#pragma once
#include "plane.h"

namespace C3D
{
    struct C3D_API Frustum
    {
        Frustum() = default;
        Frustum(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, f32 aspect, f32 fov, f32 near, f32 far);

        void Create(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, f32 aspect, f32 fov, f32 near, f32 far);

        [[nodiscard]] bool IntersectsWithSphere(const Sphere& sphere) const;

        [[nodiscard]] bool IntersectsWithAABB(const AABB& aabb) const;

        Plane3D sides[6] = {};
    };
}  // namespace C3D