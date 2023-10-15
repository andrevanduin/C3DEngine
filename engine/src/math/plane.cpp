
#include "plane.h"
#include "c3d_math.h"

#include <glm/glm.hpp>

namespace C3D
{
	Plane3D::Plane3D() : normal(), distance() {}

	Plane3D::Plane3D(const vec3& p1, const vec3& norm)
		: normal(normalize(norm)), distance(0.0f)
	{
		distance = dot(normal, p1);
	}

	Plane3D::Plane3D(const float a, const float b, const float c, const float d)
		: normal(normalize(vec3(a, b, c))), distance(d)
	{}

	f32 Plane3D::SignedDistance(const vec3& position) const
	{
		return dot(normal, position) - distance;
	}

	bool Plane3D::IntersectsWithSphere(const Sphere& sphere) const
	{
		return SignedDistance(sphere.center) > -sphere.radius;
	}

	bool Plane3D::IntersectsWithAABB(const AABB& aabb) const
	{
		const f32 r = aabb.extents.x * Abs(normal.x) + aabb.extents.y * Abs(normal.y) + aabb.extents.z * Abs(normal.z);
		return -r <= SignedDistance(aabb.center);
	}
}