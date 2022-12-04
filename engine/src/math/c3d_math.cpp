
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

	f64 Abs(const f64 x)
	{
		return abs(x);
	}
}
