
#include "frustum.h"

namespace C3D
{
    enum FrustumPlane
    {
        FrustumPlaneTop,
        FrustumPlaneBottom,
        FrustumPlaneRight,
        FrustumPlaneLeft,
        FrustumPlaneFar,
        FrustumPlaneNear,
    };
};

namespace C3D
{
    Frustum::Frustum(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, const f32 aspect, const f32 fov,
                     const f32 near, const f32 far)
    {
        Create(position, forward, right, up, aspect, fov, near, far);
    }

    void Frustum::Create(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, const f32 aspect, const f32 fov,
                         const f32 near, const f32 far)
    {
        const f32 halfV       = far * tanf(fov * 0.5f);
        const f32 halfH       = halfV * aspect;
        const vec3 forwardFar = forward * far;

        sides[FrustumPlaneTop]    = Plane3D((forward * near) + position, forward);
        sides[FrustumPlaneBottom] = Plane3D(position + forwardFar, -1.0f * forward);
        sides[FrustumPlaneRight]  = Plane3D(position, glm::cross(up, forwardFar + (right * halfH)));
        sides[FrustumPlaneLeft]   = Plane3D(position, glm::cross(forwardFar - (right * halfH), up));
        sides[FrustumPlaneFar]    = Plane3D(position, glm::cross(right, forwardFar - (up * halfV)));
        sides[FrustumPlaneNear]   = Plane3D(position, glm::cross(forwardFar + (up * halfV), right));
    }

    bool Frustum::IntersectsWithSphere(const Sphere& sphere) const
    {
        for (auto& side : sides)
        {
            if (!side.IntersectsWithSphere(sphere))
            {
                return false;
            }
        }
        return true;
    }

    bool Frustum::IntersectsWithAABB(const AABB& aabb) const
    {
        for (const auto& side : sides)
        {
            if (!side.IntersectsWithAABB(aabb))
            {
                return false;
            }
        }
        return true;
    }
}  // namespace C3D
