
#pragma once

#include "containers/string.h"
#include "core/frame_data.h"
#include "core/uuid.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"
#include "terrain_config.h"

namespace C3D
{
    class SystemManager;
    class Terrain;

    struct Material;

    class C3D_API TerrainChunk
    {
    public:
        TerrainChunk() = default;

        void Initialize(const Terrain& terrain);

        void Load(const Terrain& terrain, u32 chunkIndex, u32 globalBaseIndex, u32 chunkRowCount, u32 chunkColumnCount);

        void SetMaterial(Material* material);

        void Unload();

        void Destroy();

        Geometry* GetGeometry() { return &m_geometry; }

    private:
        DynamicArray<TerrainVertex> m_vertices;
        DynamicArray<u32> m_indices;

        Extents3D m_extents;
        Geometry m_geometry;
    };

    class C3D_API Terrain
    {
    public:
        Terrain() = default;

        bool Create(const TerrainConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Update();

        void Destroy();

        UUID GetId() const { return m_id; }

        const String& GetName() const { return m_name; }

        mat4 GetModel() const { return m_transform.GetWorld(); }

        Transform* GetTransform() { return &m_transform; }

        const DynamicArray<TerrainChunk>& GetChunks() const { return m_chunks; }

        void SetTransform(const Transform& t) { m_transform = t; }

    private:
        bool LoadFromResource();

        void LoadJobSuccess();

        void LoadJobFailure();

        UUID m_id;
        String m_name;

        u32 m_tileCountX = 0, m_tileCountZ = 0;
        u32 m_totalTileCount = 0, m_vertexCount = 0;

        /** @brief The size of an individual chunk (is always square). */
        u32 m_chunkSize = 1;

        /** @brief The scale of each individual tile on the x and z axis. */
        f32 m_tileScaleX = 1.0f, m_tileScaleY = 1.0f, m_tileScaleZ = 1.0f;

        Transform m_transform;
        Extents3D m_extents;
        vec3 m_origin;

        /** @brief The chunks that make up this terrain. */
        DynamicArray<TerrainChunk> m_chunks;

        /** @brief The configuration describing what this terrain should look like. */
        TerrainConfig m_config;

        friend class TerrainChunk;
    };
}  // namespace C3D