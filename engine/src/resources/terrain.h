
#pragma once

#include "containers/string.h"
#include "core/defines.h"
#include "core/frame_data.h"
#include "geometry.h"
#include "material.h"
#include "math/math_types.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"
#include "resources/scene/simple_scene_config.h"

namespace C3D
{
    class SystemManager;

    struct TerrainConfig
    {
        TerrainConfig() {}

        TerrainConfig(const SimpleSceneTerrainConfig& cfg) : name(cfg.name), resourceName(cfg.resourceName) {}

        String name, resourceName;
        u32 tileCountX = 0.0f, tileCountZ = 0.0f;
        f32 tileScaleX = 0.0f, tileScaleZ = 0.0f;

        DynamicArray<String> materials;
    };

    class Terrain
    {
    public:
        Terrain();

        bool Create(const SystemManager* pSystemsManager, const TerrainConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Update();

        bool Render(FrameData& frameData, const mat4& projection, const mat4& view, const mat4& model,
                    const vec4& ambientColor, const vec3& viewPosition, u32 renderMode);

        void Destroy();

        mat4 GetModel() const { return m_transform.GetWorld(); }

        Geometry* GetGeometry() { return &m_geometry; }

        const String& GetName() const { return m_name; }

        void SetTransform(const Transform& t) { m_transform = t; }

    private:
        String m_name;
        u32 m_tileCountX = 0, m_tileCountZ = 0;
        u32 m_totalTileCount = 0, m_vertexCount = 0;

        /** @brief The scale of each individual tile on the x and z axis. */
        f32 m_tileScaleX = 1.0f, m_tileScaleZ = 1.0f;

        Transform m_transform;
        Extents3D m_extents;
        vec3 m_origin;

        Geometry m_geometry;

        DynamicArray<TerrainVertex> m_vertices;
        DynamicArray<u32> m_indices;
        DynamicArray<Material*> m_materials;

        TerrainConfig m_config;
        LoggerInstance<16> m_logger;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D