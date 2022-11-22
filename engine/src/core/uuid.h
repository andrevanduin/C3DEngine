
#pragma once
#include <random>

#include "defines.h"

namespace C3D
{
	/* @brief A universally unique identifier (UUID). */
	struct UUID
	{
		char value[37];
	};

	class UUIDS
	{
	public:
		static void Seed(u64 seed);

		static UUID Generate();
	};
}