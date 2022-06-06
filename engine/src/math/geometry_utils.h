
#pragma once
#include "core/defines.h"
#include "renderer/vertex.h"

namespace C3D::GeometryUtils
{
	void GenerateNormals(Vertex3D* vertices, u32 indexCount, const u32* indices);

	void GenerateTangents(Vertex3D* vertices, u32 indexCount, const u32* indices);
}
