
#include "Math.h"

namespace C3D
{
	uint32_t Math::PreviousPow2(const uint32_t number)
	{
		uint32_t r = 1;
		while (r * 2 < number) r *= 2;
		return r;
	}
}
