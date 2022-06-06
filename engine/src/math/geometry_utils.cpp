
#include "geometry_utils.h"

namespace C3D::GeometryUtils
{
	void GenerateNormals(Vertex3D* vertices, const u32 indexCount, const u32* indices)
	{
		for (u32 i = 0; i < indexCount; i += 3)
		{
			const u32 i0 = indices[i + 0];
			const u32 i1 = indices[i + 1];
			const u32 i2 = indices[i + 2];

			const vec3 edge1 = vertices[i1].position - vertices[i0].position;
			const vec3 edge2 = vertices[i2].position - vertices[i0].position;

			const vec3 normal = normalize(cross(edge1, edge2));

			// NOTE: This is a simple surface normal. Smoothing out should be done separately if required.
			vertices[i0].normal = normal;
			vertices[i1].normal = normal;
			vertices[i2].normal = normal;
		}
	}

	void GenerateTangents(Vertex3D* vertices, const u32 indexCount, const u32* indices)
	{
		for (u32 i = 0; i < indexCount; i += 3)
		{
			const u32 i0 = indices[i + 0];
			const u32 i1 = indices[i + 1];
			const u32 i2 = indices[i + 2];

			const vec3 edge1 = vertices[i1].position - vertices[i0].position;
			const vec3 edge2 = vertices[i2].position - vertices[i0].position;

			const f32 deltaU1 = vertices[i1].texture.x - vertices[i0].texture.x;
			const f32 deltaV1 = vertices[i1].texture.y - vertices[i0].texture.y;

			const f32 deltaU2 = vertices[i2].texture.x - vertices[i0].texture.x;
			const f32 deltaV2 = vertices[i2].texture.y - vertices[i0].texture.y;

			const f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
			const f32 fc = 1.0f / dividend;

			vec3 tangent =
			{
				fc * (deltaV2 * edge1.x - deltaV1 * edge2.x),
				fc * (deltaV2 * edge1.y - deltaV1 * edge2.y),
				fc * (deltaV2 * edge1.z - deltaV1 * edge2.z),
			};

			tangent = normalize(tangent);

			const f32 sx = deltaU1, sy = deltaU2;
			const f32 tx = deltaV1, ty = deltaV2;
			const f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

			const auto t4 = vec4(tangent, handedness);

			vertices[i0].tangent = t4;
			vertices[i1].tangent = t4;
			vertices[i2].tangent = t4;
		}
	}
}
