
#pragma once
#include "material.h"

namespace C3D
{
	constexpr auto GEOMETRY_NAME_MAX_LENGTH = 256;
	
	struct Geometry
	{
		u32 id;
		u32 internalId;
		u16 generation;

		vec3 center;
		Extents3D extents;

		CString<GEOMETRY_NAME_MAX_LENGTH> name;
		Material* material;
	};
}