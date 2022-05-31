
#pragma once
#include "material.h"

namespace C3D
{
	constexpr auto GEOMETRY_NAME_MAX_LENGTH = 256;
	
	struct Geometry
	{
		u32 id;
		u32 internalId;
		u32 generation;

		char name[GEOMETRY_NAME_MAX_LENGTH];
		Material* material;
	};
}