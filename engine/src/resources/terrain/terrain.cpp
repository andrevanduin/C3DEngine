
#include "terrain.h"

#include "core/colors.h"
#include "core/random.h"
#include "core/scoped_timer.h"
#include "math/c3d_math.h"
#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/jobs/job_system.h"
#include "systems/lights/light_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D
{
    void TerrainChunkLod::Initialize(const Terrain& terrain, u32 index)
    {
        u32 tileStride = terrain.m_chunkSize;
        if (index != 0)
        {
            tileStride = terrain.m_chunkSize * (1.0f / (index * 2));
        }

        m_surfaceIndexCount = tileStride * tileStride * 6;
        u32 totalIndexCount = m_surfaceIndexCount + (tileStride * 6 * 4);
        m_indices.Resize(totalIndexCount);
    }

    void TerrainChunkLod::GenerateIndices(const Terrain& terrain, const TerrainChunk& chunk, u32 index)
    {
        // Generate indices for the surface
        u32 chunkStride = terrain.m_chunkSize + 1;
        u32 lodSkipRate = (1 << index);

        for (u32 row = 0, i = 0; row < terrain.m_chunkSize; row += lodSkipRate)
        {
            for (u32 col = 0; col < terrain.m_chunkSize; col += lodSkipRate, i += 6)
            {
                u32 nextRow = row + lodSkipRate;
                u32 nextCol = col + lodSkipRate;

                u32 v0 = (row * chunkStride) + col;
                u32 v1 = (row * chunkStride) + nextCol;
                u32 v2 = (nextRow * chunkStride) + col;
                u32 v3 = (nextRow * chunkStride) + nextCol;

                m_indices[i + 0] = v2;
                m_indices[i + 1] = v1;
                m_indices[i + 2] = v0;
                m_indices[i + 3] = v3;
                m_indices[i + 4] = v1;
                m_indices[i + 5] = v2;
            }
        }

        // Generate indices for the skirts
        u32 ii = m_surfaceIndexCount;
        u32 vi = chunk.m_surfaceVertexCount;

        // Order is left, right, top and finally bottom
        for (u8 s = 0; s < TSS_MAX; ++s)
        {
            // Iterate vertices at the lod skip rate
            for (u32 i = 0; i < terrain.m_chunkSize; i += lodSkipRate, ii += 6, vi += lodSkipRate)
            {
                // Find the 2 verts along the surface's edge
                u32 v0, v1;
                if (s == TSS_LEFT)
                {
                    v0 = i * chunkStride;
                    v1 = (i + lodSkipRate) * chunkStride;
                }
                else if (s == TSS_RIGHT)
                {
                    v0 = (i * chunkStride) + (chunkStride - 1);
                    v1 = ((i + lodSkipRate) * chunkStride) + (chunkStride - 1);
                }
                else if (s == TSS_TOP)
                {
                    v0 = i;
                    v1 = i + lodSkipRate;
                }
                else  // s == TSS_BOTTOM
                {
                    v0 = i + (chunkStride * terrain.m_chunkSize);
                    v1 = (i + lodSkipRate) + (chunkStride * terrain.m_chunkSize);
                }

                // The other 2 verts are directly below
                u32 v2 = vi;
                u32 v3 = vi + lodSkipRate;

                if (s == TSS_LEFT || s == TSS_BOTTOM)
                {
                    // Counter-clockwise for left and bottom
                    m_indices[ii + 0] = v0;
                    m_indices[ii + 1] = v3;
                    m_indices[ii + 2] = v1;
                    m_indices[ii + 3] = v0;
                    m_indices[ii + 4] = v2;
                    m_indices[ii + 5] = v3;
                }
                else  // s == TSS_RIGHT || TSS_TOP
                {
                    // Clockwise for right and top
                    m_indices[ii + 0] = v0;
                    m_indices[ii + 1] = v1;
                    m_indices[ii + 2] = v2;
                    m_indices[ii + 3] = v1;
                    m_indices[ii + 4] = v3;
                    m_indices[ii + 5] = v2;
                }
            }

            // Skip the last vertex since the loop above uses i and i + 1
            vi++;
        }
    }

    bool TerrainChunkLod::UploadIndices()
    {
        const u32 totalSize = m_indices.Size() * sizeof(u32);
        if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Index, totalSize, m_indexBufferOffset))
        {
            ERROR_LOG("Failed to allocate memory for LOD index data.");
            return false;
        }

        // TODO: Passing false here produces a queue wait and should be offloaded to another queue
        if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Index, m_indexBufferOffset, totalSize, m_indices.GetData(), false))
        {
            ERROR_LOG("Failed to load index data for LOD.");
            return false;
        }

        return true;
    }

    bool TerrainChunkLod::FreeIndices()
    {
        return Renderer.FreeInRenderBuffer(RenderBufferType::Index, m_indices.Size() * sizeof(u32), m_indexBufferOffset);
    }

    void TerrainChunkLod::Destroy()
    {
        m_surfaceIndexCount = 0;
        m_indexBufferOffset = 0;
        m_indices.Destroy();
    }

    void TerrainChunk::Initialize(const Terrain& terrain)
    {
        const auto chunkStride = terrain.m_chunkSize + 1;
        m_surfaceVertexCount   = chunkStride * chunkStride;

        // We also add a row of vertices on all sides of the chunk (skirt).
        u32 totalVertexCount = m_surfaceVertexCount + (chunkStride * 4);

        m_vertices.Resize(totalVertexCount);
        m_lods.Resize(terrain.m_numberOfLods);

        for (u32 i = 0; i < m_lods.Size(); ++i)
        {
            m_lods[i].Initialize(terrain, i);
        }
    }

    bool TerrainChunk::SetCurrentLOD(u32 lod)
    {
        if (lod >= m_lods.Size())
        {
            ERROR_LOG("Could not set the LOD since it's outside this chunk's range of LODs ({} >= {})", lod, m_lods.Size());
            return false;
        }

        m_currentLod = lod;
        return true;
    }

    void TerrainChunk::Load(const Terrain& terrain, u32 xOffset, u32 zOffset)
    {
        auto timer = ScopedTimer(String::FromFormat("Loading chunk at: {}, {}", xOffset, zOffset));

        f32 xPos = xOffset * terrain.m_chunkSize * terrain.m_tileScaleX;
        f32 zPos = zOffset * terrain.m_chunkSize * terrain.m_tileScaleZ;

        // Account for extra row and column on each chunk (to fix overlap)
        u32 chunkStride = terrain.m_chunkSize + 1;

        f32 yMin = FLT_MAX;
        f32 yMax = FLT_MIN;

        // Generate our vertices
        for (u32 z = 0, i = 0; z < chunkStride; ++z)
        {
            for (u32 x = 0; x < chunkStride; ++x, ++i)
            {
                auto& vert      = m_vertices[i];
                vert.position.x = xPos + (x * terrain.m_tileScaleX);
                vert.position.z = zPos + (z * terrain.m_tileScaleZ);

                u32 globalX = x + (xOffset * chunkStride);
                if (xOffset > 0)
                {
                    globalX -= xOffset;
                }
                u32 globalZ = z + (zOffset * chunkStride);
                if (zOffset > 0)
                {
                    globalZ -= zOffset;
                }

                u32 globalTerrainIndex = globalX + (globalZ * (terrain.m_tileCountX + 1));
                const auto height      = terrain.m_config.vertexConfigs[globalTerrainIndex].height;

                vert.position.y = height * terrain.m_tileScaleY;
                vert.color      = WHITE;
                vert.normal     = { 0, 1, 0 };
                vert.texture    = vec2(static_cast<f32>(xOffset + x), static_cast<f32>(zOffset + z));

                yMin = C3D::Min(yMin, vert.position.y);
                yMax = C3D::Max(yMax, vert.position.y);

                vert.materialWeights[0] = AttenuationMinMax(-0.2f, 0.2f, height);
                vert.materialWeights[1] = AttenuationMinMax(0.0f, 0.3f, height);
                vert.materialWeights[2] = AttenuationMinMax(0.15f, 0.9f, height);
                vert.materialWeights[3] = AttenuationMinMax(0.5f, 1.2f, height);
            }
        }

        u32 targetVertexIndex = m_surfaceVertexCount;
        for (u8 s = 0; s < TSS_MAX; ++s)
        {
            for (u32 i = 0; i < chunkStride; ++i, ++targetVertexIndex)
            {
                TerrainVertex* source;
                if (s == TSS_LEFT)
                {
                    source = &m_vertices[i * chunkStride];
                }
                else if (s == TSS_RIGHT)
                {
                    source = &m_vertices[i * chunkStride + terrain.m_chunkSize];
                }
                else if (s == TSS_TOP)
                {
                    source = &m_vertices[i];
                }
                else  // s == TSS_BOTTOM
                {
                    source = &m_vertices[i + (chunkStride * terrain.m_chunkSize)];
                }

                // Copy the source vertex data to the target
                m_vertices[targetVertexIndex] = *source;
                // But lower it's height
                m_vertices[targetVertexIndex].position.y -= 0.1f * terrain.m_tileScaleY;
            }
        }

        vec3 min = m_vertices.First().position;
        min.y    = yMin;

        vec3 max = m_vertices.Last().position;
        max.y    = yMax;

        m_center      = (min + max) * 0.5f;
        m_extents.min = min;
        m_extents.max = max;

        // Generate our LODs
        for (u32 i = 0; i < m_lods.Size(); ++i)
        {
            m_lods[i].GenerateIndices(terrain, *this, i);
        }

        const auto& firstLod = m_lods.First();

        // Generate normals and tangents only for the first LOD
        GeometryUtils::GenerateNormals(m_vertices, firstLod.GetIndices(), firstLod.GetSurfaceIndexCount());
        GeometryUtils::GenerateTerrainTangents(m_vertices, firstLod.GetIndices(), firstLod.GetSurfaceIndexCount());

        const auto totalSize = m_vertices.Size() * sizeof(TerrainVertex);
        if (!Renderer.AllocateInRenderBuffer(RenderBufferType::Vertex, totalSize, m_vertexBufferOffset))
        {
            ERROR_LOG("Failed to allocate space for the vertex buffer.");
            return;
        }

        // TODO: Passing false here produces a queue wait and should be offloaded to another queue
        if (!Renderer.LoadRangeInRenderBuffer(RenderBufferType::Vertex, m_vertexBufferOffset, totalSize, m_vertices.GetData(), false))
        {
            ERROR_LOG("Failed to load vertices into vertex buffer.");
            return;
        }

        for (auto& lod : m_lods)
        {
            if (!lod.UploadIndices())
            {
                ERROR_LOG("Failed to upload LOD indices.");
                return;
            }
        }

        generation++;
    }

    void TerrainChunk::Unload()
    {
        if (!Renderer.FreeInRenderBuffer(RenderBufferType::Vertex, sizeof(TerrainVertex) * m_vertices.Size(), m_vertexBufferOffset))
        {
            ERROR_LOG("Failed to free vertices from buffer.");
        }

        for (auto& lod : m_lods)
        {
            if (!lod.FreeIndices())
            {
                ERROR_LOG("Failed to free indices from for LOD.");
            }
        }
    }

    void TerrainChunk::Destroy()
    {
        m_vertices.Destroy();

        for (auto& lod : m_lods)
        {
            lod.Destroy();
        }
    }

    bool Terrain::Create(const TerrainConfig& config)
    {
        m_config = config;
        m_name   = config.name;

        return true;
    }

    bool Terrain::Initialize()
    {
        m_extents.min = { 0, 0, 0 };
        m_extents.max = { 0, 0, 0 };
        m_origin      = { 0, 0, 0 };

        return true;
    }

    bool Terrain::Load()
    {
        if (m_config.resourceName)
        {
            LoadFromResource();
            return true;
        }

        // NOTE: no config so we will use defaults
        m_tileCountX = 100;
        m_tileCountZ = 100;

        m_tileScaleX = 1.0f;
        m_tileScaleZ = 1.0f;

        m_totalTileCount = m_tileCountX * m_tileCountZ;
        m_vertexCount    = m_totalTileCount;

        return true;
    }

    bool Terrain::Unload()
    {
        // Invalidate this terrain so it no longer gets rendered
        m_id.Invalidate();

        // Unload all chunks
        for (auto& chunk : m_chunks)
        {
            chunk.Unload();
        }

        // If we have a material we release it
        if (m_material)
        {
            Materials.Release(m_material->name);
            m_material = nullptr;
        }

        return true;
    }

    bool Terrain::Update() { return true; }

    void Terrain::Destroy()
    {
        if (m_id)
        {
            // If our id is not invalidated yet we must first unload
            Unload();
        }

        for (auto& chunk : m_chunks)
        {
            chunk.Destroy();
        }

        m_config.Destroy();

        m_tileScaleX = 0;
        m_tileScaleY = 0;
        m_tileScaleZ = 0;

        m_tileCountX = 0;
        m_tileCountZ = 0;

        m_origin  = vec3(0);
        m_extents = { vec3(0), vec3(0) };
    }

    void Terrain::LoadFromResource()
    {
        Jobs.Submit([this]() { return LoadJobEntry(); }, [this]() { LoadJobSuccess(); }, [this]() { LoadJobFailure(); });
    }

    bool Terrain::LoadJobEntry()
    {
        auto timer = ScopedTimer("Loading Terrain Resource");
        return Resources.Read(m_config.resourceName, m_config);
    }

    void Terrain::LoadJobSuccess()
    {
        if (m_config.tileCountX == 0)
        {
            ERROR_LOG("TileCountX must > 0.");
            return;
        }

        if (m_config.tileCountZ == 0)
        {
            ERROR_LOG("TileCountZ must be > 0.");
            return;
        }

        if (m_config.tileScaleX <= 0.0f)
        {
            ERROR_LOG("TileScaleX must be > 0.");
            return;
        }

        if (m_config.tileScaleZ <= 0.0f)
        {
            ERROR_LOG("TileScaleZ must be > 0.");
            return;
        }

        m_tileCountX = m_config.tileCountX;
        m_tileCountZ = m_config.tileCountZ;

        m_tileScaleX = m_config.tileScaleX;
        m_tileScaleY = m_config.tileScaleY;
        m_tileScaleZ = m_config.tileScaleZ;

        m_chunkSize = m_config.chunkSize;
        // The number of lods is equal to however many times we can divide the size of a chunk by 2
        // Then we floor and add 1 to ensure that we always have at least 1 LOD
        m_numberOfLods = Floor(Log2(m_chunkSize)) + 1;

        m_totalTileCount = m_tileCountX * m_tileCountZ;
        m_vertexCount    = m_totalTileCount;

        u32 chunkRowCount    = m_tileCountZ / m_chunkSize;
        u32 chunkColumnCount = m_tileCountX / m_chunkSize;

        {
            auto timer = ScopedTimer("Initializing Chunks");

            // Resize our chunks array
            m_chunks.Resize(chunkRowCount * chunkColumnCount);
            // Initialize all chunks
            for (auto& chunk : m_chunks)
            {
                chunk.Initialize(*this);
            }
        }

        {
            auto timer = ScopedTimer("Loading Chunks");

            // Load all our chunks
            for (u32 z = 0, i = 0; z < chunkRowCount; ++z)
            {
                for (u32 x = 0; x < chunkColumnCount; ++x, ++i)
                {
                    m_chunks[i].Load(*this, x, z);
                }
            }
        }

        {
            auto timer = ScopedTimer("Acquiring Terrain Material");

            String terrainMaterialName = String::FromFormat("terrain_mat_{}", m_name);
            Material* material         = Materials.AcquireTerrain(terrainMaterialName, m_config.materials, true);
            if (!material)
            {
                WARN_LOG("Failed to acquire terrain material. Using default instead.");
                material = Materials.GetDefaultTerrain();
            }

            m_material = material;
        }

        {
            auto timer = ScopedTimer("Unloading Terrain Config");

            Resources.Cleanup(m_config);
        }

        m_id.Generate();
    }

    void Terrain::LoadJobFailure()
    {
        ERROR_LOG("Failed to load: '{}'.", m_config.resourceName);

        Resources.Cleanup(m_config);
    }
}  // namespace C3D