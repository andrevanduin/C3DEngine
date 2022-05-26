
#include "c3d_math.h"
#include "platform/platform.h"

#include <cmath>
#include <cstdlib>

namespace C3D
{
	static bool RAND_SEEDED = false;

	f32 Sin(const f32 x)
	{
		return sinf(x);
	}

	f32 Cos(const f32 x)
	{
		return cosf(x);
	}

	f32 Tan(const f32 x)
	{
		return tanf(x);
	}

	f32 ACos(const f32 x)
	{
		return acosf(x);
	}

	f32 Sqrt(const f32 x)
	{
		return sqrtf(x);
	}

	f32 Abs(const f32 x)
	{
		return abs(x);
	}

	i32 Random()
	{
		if (!RAND_SEEDED)
		{
			srand(static_cast<u32>(Platform::GetAbsoluteTime()));
			RAND_SEEDED = true;
		}
		return rand();
	}

	i32 RandomInRange(const i32 min, const i32 max)
	{
		if (!RAND_SEEDED)
		{
			srand(static_cast<u32>(Platform::GetAbsoluteTime()));
			RAND_SEEDED = true;
		}
		return (rand() % (max - min + 1)) + min;
	}

	f32 RandomF()
	{
		return Random() / static_cast<f32>(RAND_MAX);
	}

	f32 RandomInRangeF(const f32 min, const f32 max)
	{
		return min + (static_cast<f32>(Random()) / (static_cast<f32>(RAND_MAX) / (max - min)));
	}
}
