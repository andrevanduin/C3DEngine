
#pragma once
#include "geometry.h"
#include "renderer/transform.h"

namespace C3D
{
	struct Mesh
	{
		u16 geometryCount;
		Geometry** geometries;

		Transform transform;
	};
}
