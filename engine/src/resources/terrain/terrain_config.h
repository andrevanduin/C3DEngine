
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "resources/resource_types.h"

namespace C3D
{
    constexpr u8 TERRAIN_MAX_MATERIAL_COUNT = 4;

    struct TerrainVertexConfig
    {
        f32 height = 0.0f;

        TerrainVertexConfig() = default;

        TerrainVertexConfig(f32 height) : height(height) {}
    };

    struct TerrainConfig final : Resource
    {
        TerrainConfig() = default;

        void Destroy()
        {
            name.Destroy();
            resourceName.Destroy();
            vertexConfigs.Destroy();
            materials.Destroy();
        }

        String name;
        String resourceName;

        u32 tileCountX = 0;
        u32 tileCountZ = 0;

        f32 tileScaleX = 1.0f;
        f32 tileScaleY = 1.0f;
        f32 tileScaleZ = 1.0f;
        f32 scaleY     = 1.0f;

        DynamicArray<TerrainVertexConfig> vertexConfigs;
        DynamicArray<String> materials;
    };
}  // namespace C3D