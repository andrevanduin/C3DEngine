
#pragma once
#include "core/defines.h"
#include "renderer/vertex.h"
#include "systems/geometry/geometry_system.h"

namespace C3D::GeometryUtils
{
    template <typename T>
    void GenerateNormals(DynamicArray<T>& vertices, const DynamicArray<u32>& indices)
    {
        for (u32 i = 0; i < indices.Size(); i += 3)
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

    void GenerateTangents(DynamicArray<Vertex3D>& vertices, const DynamicArray<u32>& indices);
    void GenerateTerrainTangents(DynamicArray<TerrainVertex>& vertices, const DynamicArray<u32>& indices);

    void DeduplicateVertices(GeometryConfig& config);

}  // namespace C3D::GeometryUtils
