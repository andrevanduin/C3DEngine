
#include "plane.h"

#include <glm/glm.hpp>

namespace C3D
{
	Plane3D::Plane3D() : normal(), distance() {}

	Plane3D::Plane3D(const vec3& p1, const vec3& norm)
		: normal(normalize(norm)), distance(0.0f)
	{
		distance = dot(normal, p1);
	}

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
		const f32 r = aabb.extents.x * abs(normal.x) + aabb.extents.y * abs(normal.y) + aabb.extents.z * abs(normal.z);
		return -r <= SignedDistance(aabb.center);
	}
}