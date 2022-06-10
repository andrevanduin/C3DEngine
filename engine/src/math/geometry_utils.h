
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "renderer/vertex.h"
#include "systems/geometry_system.h"

namespace C3D::GeometryUtils
{
	void GenerateNormals(Vertex3D* vertices, u32 indexCount, const u32* indices);

	void GenerateTangents(Vertex3D* vertices, u64 indexCount, const u32* indices);

	void DeduplicateVertices(GeometryConfig<Vertex3D, u32>& config);
}
