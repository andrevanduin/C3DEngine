
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
    constexpr const char* INSTANCE_NAME = "TERRAIN";

    void TerrainChunk::Initialize(const Terrain& terrain)
    {
        const auto sizeSq = (terrain.m_chunkSize + 1) * (terrain.m_chunkSize + 1);

        m_vertices.Resize(sizeSq);
        m_indices.Resize(sizeSq * 6);
    }

    void TerrainChunk::Load(const Terrain& terrain, u32 xOffset, u32 zOffset)
    {
        f32 xPos = xOffset * terrain.m_tileScaleX * terrain.m_chunkSize;
        f32 zPos = zOffset * terrain.m_tileScaleZ * terrain.m_chunkSize;

        // Account for extra row and column on each chunk (to fix overlap)
        u32 chunkStride = terrain.m_chunkSize + 1;

        {
            auto timer = ScopedTimer("Generating TerrainChunk vertices");

            // Generate our vertices
            u32 globalTerrainIndexStart = (zOffset * terrain.m_tileCountX * terrain.m_chunkSize) + (xOffset * terrain.m_chunkSize);

            f32 yMin = FLT_MAX;
            f32 yMax = FLT_MIN;

            for (u32 z = 0, i = 0; z < chunkStride; z++)
            {
                for (u32 x = 0; x < chunkStride; x++, i++)
                {
                    auto& vert = m_vertices[i];

                    u32 globalTerrainIndex = globalTerrainIndexStart + x + z * terrain.m_tileCountX;
                    const auto height      = terrain.m_config.vertexConfigs[globalTerrainIndex].height;

                    vert.position = { xPos + (x * terrain.m_tileScaleX), height * terrain.m_tileScaleY, zPos + (z * terrain.m_tileScaleZ) };
                    vert.color    = WHITE;
                    vert.normal   = { 0, 1, 0 };
                    vert.texture  = vec2(static_cast<f32>(xOffset + x), static_cast<f32>(zOffset + z));

                    yMin = C3D::Min(yMin, vert.position.y);
                    yMax = C3D::Max(yMax, vert.position.y);

                    vert.materialWeights[0] = AttenuationMinMax(-0.2f, 0.2f, height);
                    vert.materialWeights[1] = AttenuationMinMax(0.0f, 0.3f, height);
                    vert.materialWeights[2] = AttenuationMinMax(0.15f, 0.9f, height);
                    vert.materialWeights[3] = AttenuationMinMax(0.5f, 1.2f, height);
                }
            }

            vec3 min = m_vertices.First().position;
            min.y    = yMin;

            vec3 max = m_vertices.Last().position;
            max.y    = yMax;

            m_center      = min + ((max - min) * 0.5f);
            m_extents.min = min;
            m_extents.max = max;
        }

        {
            auto timer = ScopedTimer("Generating TerrainChunk indices");

            // Generate our indices
            for (u32 z = 0, i = 0; z < terrain.m_chunkSize; ++z)
            {
                for (u32 x = 0; x < terrain.m_chunkSize; ++x, i += 6)
                {
                    u32 v0 = (z * chunkStride) + x;
                    u32 v1 = (z * chunkStride) + x + 1;
                    u32 v2 = ((z + 1) * chunkStride) + x;
                    u32 v3 = ((z + 1) * chunkStride) + x + 1;

                    m_indices[i + 0] = v2;
                    m_indices[i + 1] = v1;
                    m_indices[i + 2] = v0;
                    m_indices[i + 3] = v3;
                    m_indices[i + 4] = v1;
                    m_indices[i + 5] = v2;
                }
            }
        }

        {
            auto timer = ScopedTimer("Generating TerrainChunk normals");
            GeometryUtils::GenerateNormals(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Generating TerrainChunk tangents");
            GeometryUtils::GenerateTerrainTangents(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Creating TerrainChunk Geometry");

            if (!Renderer.CreateGeometry(m_geometry, sizeof(TerrainVertex), m_vertices.Size(), m_vertices.GetData(), sizeof(u32),
                                         m_indices.Size(), m_indices.GetData()))
            {
                ERROR_LOG("Failed to create geometry.");
                return;
            }
        }

        {
            auto timer = ScopedTimer("Uploading TerrainChunk Geometry");

            if (!Renderer.UploadGeometry(m_geometry))
            {
                ERROR_LOG("Failed to upload geometry.");
                return;
            }
        }

        // m_geometry.center  = m_origin;
        m_geometry.extents = m_extents;
        // TODO: This should be done in the renderer (CreateGeometry() method)
        m_geometry.generation++;
        generation++;
    }

    void TerrainChunk::SetMaterial(Material* material) { m_geometry.material = material; }

    void TerrainChunk::Unload()
    {
        Materials.Release(m_geometry.material->name);
        Renderer.DestroyGeometry(m_geometry);
    }

    void TerrainChunk::Destroy()
    {
        m_vertices.Destroy();
        m_indices.Destroy();

        Renderer.DestroyGeometry(m_geometry);
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
            return LoadFromResource();
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
        for (auto& chunk : m_chunks)
        {
            chunk.Unload();
        }
        return true;
    }

    bool Terrain::Update() { return true; }

    void Terrain::Destroy()
    {
        m_id.Invalidate();

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

    bool Terrain::LoadFromResource()
    {
        JobInfo info;
        info.entryPoint = [this]() {
            auto timer = ScopedTimer("Loading Terrain Resource");
            return Resources.Load(m_config.resourceName, m_config);
        };
        info.onSuccess = [this]() { LoadJobSuccess(); };
        info.onFailure = [this]() { LoadJobFailure(); };

        Jobs.Submit(std::move(info));
        return true;
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

        m_totalTileCount = m_tileCountX * m_tileCountZ;
        m_vertexCount    = m_totalTileCount;

        u32 chunkRowCount    = m_tileCountX / m_chunkSize;
        u32 chunkColumnCount = m_tileCountZ / m_chunkSize;

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

            for (auto& chunk : m_chunks)
            {
                chunk.SetMaterial(material);
            }
        }

        {
            auto timer = ScopedTimer("Unloading Terrain Config");

            Resources.Unload(m_config);
        }

        m_id.Generate();
    }

    void Terrain::LoadJobFailure()
    {
        ERROR_LOG("Failed to load: '{}'.", m_config.resourceName);

        Resources.Unload(m_config);
    }
}  // namespace C3D