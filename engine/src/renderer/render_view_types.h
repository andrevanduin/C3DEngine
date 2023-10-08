
#pragma once
#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderpass.h"
#include "resources/terrain/terrain_config.h"

namespace C3D
{
    class RenderView;
    class Skybox;
    class Mesh;
    class UIMesh;
    class SystemManager;
    struct Geometry;
    struct Material;

    struct GeometryRenderData
    {
        explicit GeometryRenderData(Geometry* geometry) : geometry(geometry) {}

        GeometryRenderData(const mat4& model, Geometry* geometry, const u32 uniqueId = INVALID_ID)
            : model(model), geometry(geometry), uniqueId(uniqueId)
        {}

        mat4 model;
        Geometry* geometry;
        u32 uniqueId = INVALID_ID;
    };

    struct RenderViewWorldData
    {
        DynamicArray<GeometryRenderData> worldGeometries;
        DynamicArray<GeometryRenderData> terrainGeometries;
        DynamicArray<GeometryRenderData> debugGeometries;
    };

    struct RenderViewPacket
    {
        // @brief A constant pointer to the view this packet is associated with.
        RenderView* view;

        mat4 viewMatrix;
        mat4 projectionMatrix;
        vec3 viewPosition;
        vec4 ambientColor;

        DynamicArray<GeometryRenderData, LinearAllocator> geometries;
        DynamicArray<GeometryRenderData, LinearAllocator> terrainGeometries;
        DynamicArray<GeometryRenderData, LinearAllocator> debugGeometries;

        // @brief The name of the custom shader to use, if not using it is nullptr.
        const char* customShaderName;
        // @brief Holds a pointer to extra data understood by both the object and consuming view.
        void* extendedData;
    };

    struct RenderPacket
    {
        DynamicArray<RenderViewPacket, LinearAllocator> views;
    };

    struct MeshPacketData
    {
        DynamicArray<Mesh*, LinearAllocator> meshes;
    };

    struct UIMeshPacketData
    {
        DynamicArray<UIMesh*, LinearAllocator> meshes;
    };

    class UIText;

    struct UIPacketData
    {
        UIMeshPacketData meshData;
        // TEMP
        DynamicArray<UIText*, LinearAllocator> texts;
        // TEMP END
    };

    struct PickPacketData
    {
        DynamicArray<GeometryRenderData, LinearAllocator>* worldMeshData;
        DynamicArray<GeometryRenderData, LinearAllocator>* terrainData;

        UIMeshPacketData uiMeshData;
        u32 worldGeometryCount   = 0;
        u32 terrainGeometryCount = 0;
        u32 uiGeometryCount      = 0;

        // TEMP:
        DynamicArray<UIText*, LinearAllocator> texts;
        // TEMP END
    };

    struct SkyboxPacketData
    {
        Skybox* box;
    };

    struct PrimitivePacketData
    {
        DynamicArray<Mesh*, LinearAllocator> meshes;
    };
}  // namespace C3D