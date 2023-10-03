
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

    enum class RenderViewKnownType
    {
        World  = 0x01,
        UI     = 0x02,
        Skybox = 0x03,
        /* @brief A view that simply only renders ui and world objects for the purpose of mouse picking. */
        Pick = 0x04,
    };

    enum class RenderViewViewMatrixSource
    {
        SceneCamera = 0x01,
        UiCamera    = 0x02,
        LightCamera = 0x03,
    };

    enum class RenderViewProjectionMatrixSource
    {
        DefaultPerspective  = 0x01,
        DefaultOrthographic = 0x02,
    };

    struct RenderViewConfig
    {
        /** @brief The name of the view. */
        String name;
        /** @brief The name of the custom shader used by this view. Empty if not used. */
        String customShaderName;
        /** @brief The width of this view. Set to 0 for 100% width. */
        u16 width;
        /** @brief The height of this view. Set to 0 for 100% height. */
        u16 height;
        /** @brief The known type of the view. Used to associate with view logic. */
        RenderViewKnownType type;
        /** @brief The source of the view matrix. */
        RenderViewViewMatrixSource viewMatrixSource;
        /** @brief The source of the projection matrix. */
        RenderViewProjectionMatrixSource projectionMatrixSource;
        /** @brief The number of renderPasses used in this view. */
        u8 passCount;
        /** @brief The configurations for the renderPasses used in this view. */
        DynamicArray<RenderPassConfig> passes;
        /** @brief A const pointer to our systems manager. */
        const SystemManager* pSystemsManager;
    };

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

        UIMeshPacketData uiMeshData;
        u32 uiGeometryCount;
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