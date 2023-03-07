
#include "frustum.h"

namespace C3D
{
	Frustum::Frustum() : sides() {}

	Frustum::Frustum(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, const f32 aspect, const f32 fov, const f32 near, const f32 far)
		: sides()
	{
		Create(position, forward, right, up, aspect, fov, near, far);
	}

	void Frustum::Create(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, const f32 aspect, const f32 fov, const f32 near, const f32 far)
	{
		const f32 halfV = far * tanf(fov * 0.5f);
		const f32 halfH = halfV * aspect;
		const vec3 forwardFar = forward * far;

		const vec3 rightMulHalfH = right * halfH;
		const vec3 upMulHalfV = up * halfV;

		sides[near_plane]	= Plane3D(position + forward * near, forward);
		sides[far_plane]	= Plane3D(position + forwardFar, -forward);
		sides[right_plane]	= Plane3D(position, cross(forwardFar - rightMulHalfH, up));
		sides[left_plane]	= Plane3D(position, cross(up, forwardFar + rightMulHalfH));
		sides[top_plane]	= Plane3D(position, cross(right, forwardFar - upMulHalfV));
		sides[bottom_plane]	= Plane3D(position, cross(forwardFar + upMulHalfV, right));
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
		for (auto& side : sides)
		{
			if (!side.IntersectsWithAABB(aabb))
			{
				return false;
			}
		}
		return true;
	}
}
