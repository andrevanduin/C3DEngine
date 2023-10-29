
#pragma once
#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderer_types.h"
#include "renderer/renderpass.h"
#include "resources/terrain/terrain_config.h"

namespace C3D
{
    class RenderView;
    class Mesh;
    class UIMesh;
    class SystemManager;
    class Viewport;
    struct Geometry;
    struct Material;

    struct GeometryRenderData
    {
        explicit GeometryRenderData(Geometry* geometry) : geometry(geometry) {}

        GeometryRenderData(const mat4& model, Geometry* geometry, const u32 uniqueId = INVALID_ID, bool windingInverted = false)
            : model(model), geometry(geometry), uniqueId(uniqueId), windingInverted(windingInverted)
        {}

        mat4 model;
        Geometry* geometry   = nullptr;
        u32 uniqueId         = INVALID_ID;
        bool windingInverted = false;
    };

    struct RenderViewWorldData
    {
        SkyboxPacketData skyboxData;

        DynamicArray<GeometryRenderData> worldGeometries;
        DynamicArray<GeometryRenderData> terrainGeometries;
        DynamicArray<GeometryRenderData> debugGeometries;
    };

    struct RenderViewPacket
    {
        /** @brief A pointer to the viewport that we should be using for this packet. */
        const Viewport* viewport;
        /** @brief A pointer to the view this packet is associated with. */
        RenderView* view;

        mat4 viewMatrix;
        mat4 projectionMatrix;
        vec3 viewPosition;
        vec4 ambientColor;

        SkyboxPacketData skyboxData;

        DynamicArray<GeometryRenderData, LinearAllocator> geometries;
        DynamicArray<GeometryRenderData, LinearAllocator> terrainGeometries;
        DynamicArray<GeometryRenderData, LinearAllocator> debugGeometries;

        /** @brief The name of the custom shader to use, if not using it is nullptr. **/
        const char* customShaderName;
        /** @brief Holds a pointer to extra data understood by both the object and consuming view. **/
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

    struct PrimitivePacketData
    {
        DynamicArray<Mesh*, LinearAllocator> meshes;
    };
}  // namespace C3D