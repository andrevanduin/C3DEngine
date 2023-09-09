
#pragma once
#include "material.h"
#include "renderer/vertex.h"

namespace C3D
{
    constexpr auto GEOMETRY_NAME_MAX_LENGTH = 256;

    template <typename VertexType, typename IndexType>
    struct IGeometryConfig
    {
        DynamicArray<VertexType> vertices;
        DynamicArray<IndexType> indices;

        vec3 center;
        vec3 minExtents;
        vec3 maxExtents;

        String name;
        String materialName;

        constexpr static u64 GetVertexSize() { return sizeof(VertexType); }
        constexpr static u64 GetIndexSize() { return sizeof(IndexType); }
    };

    using GeometryConfig   = IGeometryConfig<Vertex3D, u32>;
    using UIGeometryConfig = IGeometryConfig<Vertex2D, u32>;

    struct Geometry
    {
        u32 id         = INVALID_ID;
        u32 internalId = INVALID_ID;
        u16 generation = INVALID_ID_U16;

        vec3 center;
        Extents3D extents;

        String name;
        Material* material = nullptr;
    };
}  // namespace C3D