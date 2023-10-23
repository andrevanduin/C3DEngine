
#pragma once

#include "containers/string.h"
#include "core/frame_data.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"
#include "terrain_config.h"

namespace C3D
{
    class SystemManager;
    struct Material;

    class C3D_API Terrain
    {
    public:
        Terrain();

        bool Create(const SystemManager* pSystemsManager, const TerrainConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Update();

        void Destroy();

        const String& GetName() const { return m_name; }

        mat4 GetModel() const { return m_transform.GetWorld(); }

        Geometry* GetGeometry() { return &m_geometry; }

        Transform* GetTransform() { return &m_transform; }

        void SetTransform(const Transform& t) { m_transform = t; }

        u32 uniqueId = INVALID_ID;

    private:
        bool LoadFromResource();

        void LoadJobSuccess();

        void LoadJobFailure();

        String m_name;

        u32 m_tileCountX = 0, m_tileCountZ = 0;
        u32 m_totalTileCount = 0, m_vertexCount = 0;

        /** @brief The scale of each individual tile on the x and z axis. */
        f32 m_tileScaleX = 1.0f, m_tileScaleY = 1.0f, m_tileScaleZ = 1.0f;

        Transform m_transform;
        Extents3D m_extents;
        vec3 m_origin;

        DynamicArray<TerrainVertex> m_vertices;
        DynamicArray<u32> m_indices;

        Geometry m_geometry;

        TerrainConfig m_config;
        LoggerInstance<16> m_logger;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D