
#pragma once
#include "math_types.h"
#include "core/defines.h"

namespace C3D
{
	constexpr auto PI = 3.14159265358979323846f;
	constexpr auto PI_2 = 2.0f * PI;
	constexpr auto HALF_PI = 0.5f * PI;
	constexpr auto QUARTER_PI = 0.25f * PI;
	constexpr auto ONE_OVER_PI = 1.0f / PI;
	constexpr auto ONE_OVER_TWO_PI = 1.0f / PI_2;
	constexpr auto SQRT_TWO = 1.41421356237309504880f;
	constexpr auto SQRT_THREE = 1.73205080756887729352f;
	constexpr auto SQRT_ONE_OVER_TWO = 0.70710678118654752440f;
	constexpr auto SQRT_ONE_OVER_THREE = 0.57735026918962576450f;

	constexpr auto DEG_2_RAD_MULTIPLIER = PI / 180.0f;
	constexpr auto RAD_2_DEG_MULTIPLIER = 180.0f / PI;

	constexpr auto SEC_TO_MS_MULTIPLIER = 1000.0f;

	constexpr auto MS_TO_SEC_MULTIPLIER = 0.001f;

	constexpr auto INF = 1e30f;

	// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
	constexpr auto FLOAT_EPSILON = 1.192092896e-07f;

	C3D_INLINE bool IsPowerOf2(const u64 value)
	{
		return (value != 0) && ((value & (value - 1)) == 1);
	}

	C3D_INLINE f32 DegToRad(const f32 degrees)
	{
		return degrees * DEG_2_RAD_MULTIPLIER;
	}

	C3D_INLINE f32 RadToDeg(const f32 radians)
	{
		return radians * RAD_2_DEG_MULTIPLIER;
	}

	C3D_API f32 Sin(f32 x);
	C3D_API f32 Cos(f32 x);
	C3D_API f32 Tan(f32 x);
	C3D_API f32 ACos(f32 x);
	C3D_API f32 Sqrt(f32 x);
	C3D_API f32 Abs(f32 x);

	C3D_API i32 Random();
	C3D_API i32 RandomInRange(i32 min, i32 max);

	C3D_API f32 RandomF();
	C3D_API f32 RandomInRangeF(f32 min, f32 max);
}




