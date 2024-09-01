
#pragma once

#include "frame_data.h"
#include "identifiers/uuid.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/vertex.h"
#include "string/string.h"
#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"
#include "terrain_config.h"

namespace C3D
{
    class Terrain;

    struct Material;

    enum TerrainSkirtSide
    {
        TSS_LEFT = 0,
        TSS_RIGHT,
        TSS_TOP,
        TSS_BOTTOM,
        TSS_MAX
    };

    class TerrainChunk;

    class C3D_API TerrainChunkLod
    {
    public:
        void Initialize(const Terrain& terrain, u32 index);

        void GenerateIndices(const Terrain& terrain, const TerrainChunk& chunk, u32 index);
        bool UploadIndices();
        bool FreeIndices();

        void Destroy();

        const DynamicArray<u32>& GetIndices() const { return m_indices; }
        u64 GetIndexBufferOffset() const { return m_indexBufferOffset; }

        u32 GetSurfaceIndexCount() const { return m_surfaceIndexCount; }

    private:
        /** @brief The index count for the chunk surface (excluding side skirts). */
        u32 m_surfaceIndexCount = 0;
        /** @brief The indices for this LOD */
        DynamicArray<u32> m_indices;
        /** @brief The offset into the index buffer. */
        u64 m_indexBufferOffset = 0;
    };

    class C3D_API TerrainChunk
    {
    public:
        TerrainChunk() = default;

        void Initialize(const Terrain& terrain);

        bool SetCurrentLOD(u32 lod);

        void Load(const Terrain& terrain, u32 offsetX, u32 offsetZ);
        void Unload();

        void Destroy();

        u32 GetVertexCount() const { return m_vertices.Size(); }
        u64 GetVertexBufferOffset() const { return m_vertexBufferOffset; }

        u32 GetIndexCount() const { return m_lods[m_currentLod].GetIndices().Size(); }
        u64 GetIndexBufferOffset() const { return m_lods[m_currentLod].GetIndexBufferOffset(); }

        const Extents3D& GetExtents() const { return m_extents; }
        const vec3& GetCenter() const { return m_center; }

        constexpr u32 GetIndexSize() const { return sizeof(u32); }
        constexpr u32 GetVertexSize() const { return sizeof(TerrainVertex); }

    public:
        u8 generation = INVALID_ID_U8;

    private:
        /** @brief The vertices making up this chunk. */
        DynamicArray<TerrainVertex> m_vertices;
        /** @brief All the different LODs for this chunk. */
        DynamicArray<TerrainChunkLod> m_lods;
        /** @brief Index into the array of LODs which indicates which LOD is currently used for this chunk. */
        u32 m_currentLod = 0;
        /** @brief The number of vertices for the chunk's surface. */
        u32 m_surfaceVertexCount = 0;

        /** @brief The offset into the vertex buffer. */
        u64 m_vertexBufferOffset = 0;

        vec3 m_center;
        Extents3D m_extents;

        friend class TerrainChunkLod;
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

        Material* GetMaterial() const { return m_material; }

        const DynamicArray<TerrainChunk>& GetChunks() const { return m_chunks; }

        u32 GetNumberOfLods() const { return m_numberOfLods; }

    private:
        void LoadFromResource();

        bool LoadJobEntry();
        void LoadJobSuccess();
        void LoadJobFailure();

        UUID m_id;
        String m_name;

        u32 m_tileCountX = 0, m_tileCountZ = 0;
        u32 m_totalTileCount = 0, m_vertexCount = 0;

        /** @brief The size of an individual chunk (is always square). */
        u32 m_chunkSize = 1;
        /** @brief The number of LODs per chunk. */
        u32 m_numberOfLods = 1;

        /** @brief The scale of each individual tile on the x and z axis. */
        f32 m_tileScaleX = 1.0f, m_tileScaleY = 1.0f, m_tileScaleZ = 1.0f;

        Extents3D m_extents;
        vec3 m_origin;

        /** @brief The material used by this terrain. */
        Material* m_material;

        /** @brief The chunks that make up this terrain. */
        DynamicArray<TerrainChunk> m_chunks;

        /** @brief The configuration describing what this terrain should look like. */
        TerrainConfig m_config;

        friend class TerrainChunk;
        friend class TerrainChunkLod;
    };
}  // namespace C3D