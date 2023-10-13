
#include "ray.h"

#include "c3d_math.h"

namespace C3D
{
    Ray::Ray(const vec3& position, const vec3& dir)
    {
        origin    = position;
        direction = dir;
    }

    /*
    00 01 02 03
    04 05 06 07
    08 09 10 11
    12 13 14 15
    */

    bool Ray::TestAgainstExtents(const Extents3D& extents, const mat4& model, f32& distance) const
    {
        // Method implementation is based on the Real-Time Rendering and Essential Mathematics for Game

        // The nearest "far" intersection (within x, y and z plane pairs)
        f32 nearestFarIntersection = 0.0f;

        // The farthest "near" intersection (within x, y and z plane pairs)
        f32 farthestNearIntersection = std::numeric_limits<f32>::max();

        // Pick out the world position from the model matrix
        vec3 orientedPosWorld = GetPositionFromModel(model);

        // Transform the extents - which will orient and scale them to the model matrix
        vec4 min = vec4(extents.min, 1.0f) * model;
        vec4 max = vec4(extents.max, 1.0f) * model;

        // The distance between the world position and the ray's origin
        vec3 delta = orientedPosWorld - origin;

        // Test for intersection with the other planes perpendicular to each axis
        vec3 xAxis = GetRight(model);
        vec3 yAxis = GetUp(model);
        vec3 zAxis = GetBackward(model);

        vec3 axes[3] = { xAxis, yAxis, zAxis };
        for (u32 i = 0; i < 3; i++)
        {
            vec3& axis = axes[i];

            f32 e = glm::dot(axis, delta);
            f32 f = glm::dot(direction, axis);

            if (Abs(f) > F32_EPSILON)
            {
                // Store distances between ray origin and the ray-plane intersections in t1, and t2
                // Intersection with the "left" plane
                f32 t1 = (e + min[i]) / f;
                // Intersection with the "right" plane
                f32 t2 = (e + max[i]) / f;

                // Ensure t1 is the nearest intersection, swap if needed
                if (t1 > t2)
                {
                    std::swap(t1, t2);
                }

                // Check if our farthestNearIntersection has decreased and store if it has
                if (t2 < farthestNearIntersection)
                {
                    farthestNearIntersection = t2;
                }

                // Check if our nearestFarIntersection has increased and store if it has
                if (t1 > nearestFarIntersection)
                {
                    nearestFarIntersection = t1;
                }

                // If the "far" is closer than the "near", then we can say that there is no intersection
                if (farthestNearIntersection < nearestFarIntersection)
                {
                    return false;
                }
            }
            else
            {
                // Edge case, where the ray is almost parallel to the planes, then they don't have any intersections
                if (-e + min[i] > 0.0f || -e + max[i] < 0.0f)
                {
                    return false;
                }
            }
        }

        // Prevents intersections from within a bounding box if the ray originates from there
        if (nearestFarIntersection == 0.0f)
        {
            return false;
        }

        // We have an intersection
        distance = nearestFarIntersection;
        return true;
    }

    Ray Ray::FromScreen(const vec2& screenPos, const vec2& viewportSize, const vec3& origin, const mat4& view,
                        const mat4& projection)
    {
        Ray ray;
        ray.origin = origin;

        // We start with the screen position that is provided
        // First we get normalized device coordinates (-1:1 range that our gpu uses)
        vec3 rayNdc;
        rayNdc.x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
        rayNdc.y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;
        rayNdc.z = 1.0f;

        // Then we get the clip space out of those NDC
        vec4 rayClip = vec4(rayNdc.x, rayNdc.y, -1.0f, 1.0f);

        // Next we get the eye/camera space
        vec4 rayEye = glm::inverse(projection) * rayClip;

        // Unproject xy, change wz to "forward"
        rayEye.z = -1.0f;
        rayEye.w = 0.0f;

        // Convert to world coordinates
        ray.direction = vec3(rayEye * view);

        ray.direction = glm::normalize(ray.direction);

        return ray;
    }
}  // namespace C3D