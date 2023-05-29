
#pragma once
#include "plane.h"

namespace C3D
{
	struct C3D_API Frustum
	{
		static constexpr int near_plane		= 0;
		static constexpr int far_plane		= 1;
		static constexpr int right_plane	= 2;
		static constexpr int left_plane		= 3;
		static constexpr int top_plane		= 4;
		static constexpr int bottom_plane	= 5;

		Frustum();
		Frustum(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, f32 aspect, f32 fov, f32 near, f32 far);

		void Create(const vec3& position, const vec3& forward, const vec3& right, const vec3& up, f32 aspect, f32 fov, f32 near, f32 far);
		void Create(const mat4& mvp);

		[[nodiscard]] bool IntersectsWithSphere(const Sphere& sphere) const;

		[[nodiscard]] bool IntersectsWithAABB(const AABB& aabb) const;

		Plane3D sides[6];
	};
}