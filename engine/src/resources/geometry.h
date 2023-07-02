
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

        CString<GEOMETRY_NAME_MAX_LENGTH> name;
        CString<MATERIAL_NAME_MAX_LENGTH> materialName;

        constexpr static u64 GetVertexSize() { return sizeof(VertexType); }
        constexpr static u64 GetIndexSize() { return sizeof(IndexType); }
    };

    using GeometryConfig = IGeometryConfig<Vertex3D, u32>;
    using UIGeometryConfig = IGeometryConfig<Vertex2D, u32>;

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
}  // namespace C3D