
#include "terrain.h"

#include "core/random.h"
#include "core/scoped_timer.h"
#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/jobs/job_system.h"
#include "systems/lights/light_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "TERRAIN";

    bool Terrain::Create(const SystemManager* pSystemsManager, const TerrainConfig& config)
    {
        m_pSystemsManager = pSystemsManager;
        m_config          = config;
        m_name            = config.name;

        m_geometry.id         = INVALID_ID;
        m_geometry.generation = INVALID_ID_U16;

        return true;
    }

    bool Terrain::Initialize()
    {
        m_extents.min = { 0, 0, 0 };
        m_extents.max = { 0, 0, 0 };
        m_origin      = { 0, 0, 0 };

        m_vertices.Reserve(m_vertexCount);
        m_indices.Reserve(m_totalTileCount * 6);

        m_geometry.generation = INVALID_ID_U16;

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
        Materials.Release(m_geometry.material->name);
        Renderer.DestroyGeometry(m_geometry);
        return true;
    }

    bool Terrain::Update() { return true; }

    void Terrain::Destroy()
    {
        m_id.Invalidate();

        m_geometry.id         = INVALID_ID;
        m_geometry.generation = INVALID_ID_U16;
        m_geometry.name.Clear();

        m_vertices.Destroy();
        m_indices.Destroy();
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
            auto timer = ScopedTimer("Loading Terrain Resource", m_pSystemsManager);
            return Resources.Load(m_config.resourceName, m_config);
        };
        info.onSuccess = [this]() { LoadJobSuccess(); };
        info.onFailure = [this]() { LoadJobFailure(); };

        Jobs.Submit(std::move(info));
        return true;
    }

    void Terrain::LoadJobSuccess()
    {
        {
            auto timer = ScopedTimer("Generating Terrain Vertices", m_pSystemsManager);

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

            m_totalTileCount = m_tileCountX * m_tileCountZ;
            m_vertexCount    = m_totalTileCount;

            m_vertices.Reserve(m_config.vertexConfigs.Size());
            // Generate our vertices
            for (u32 z = 0, i = 0; z < m_tileCountZ; z++)
            {
                for (u32 x = 0; x < m_tileCountX; x++, i++)
                {
                    TerrainVertex vert = {};

                    vert.position = { x * m_tileScaleX, m_config.vertexConfigs[i].height * m_tileScaleY, z * m_tileScaleZ };
                    vert.color    = vec4(1.0f);
                    vert.normal   = { 0, 1, 0 };
                    vert.texture  = { x, z };

                    vert.materialWeights[0] = SmoothStep(0.00f, 0.25f, m_config.vertexConfigs[i].height);
                    vert.materialWeights[1] = SmoothStep(0.25f, 0.50f, m_config.vertexConfigs[i].height);
                    vert.materialWeights[2] = SmoothStep(0.50f, 0.75f, m_config.vertexConfigs[i].height);
                    vert.materialWeights[3] = SmoothStep(0.75f, 1.00f, m_config.vertexConfigs[i].height);

                    m_vertices.PushBack(vert);
                }
            }
        }

        {
            auto timer = ScopedTimer("Generating Terrain Indices", m_pSystemsManager);

            // Roughly allocate enough space for all the indices (slightly too much space)
            m_indices.Reserve(m_vertexCount * 6);
            for (u32 z = 0; z < m_tileCountZ - 1; z++)
            {
                for (u32 x = 0; x < m_tileCountX - 1; x++)
                {
                    u32 i0 = (z * m_tileCountX) + x;
                    u32 i1 = (z * m_tileCountX) + x + 1;
                    u32 i2 = ((z + 1) * m_tileCountX) + x;
                    u32 i3 = ((z + 1) * m_tileCountX) + x + 1;

                    m_indices.PushBack(i2);
                    m_indices.PushBack(i1);
                    m_indices.PushBack(i0);
                    m_indices.PushBack(i3);
                    m_indices.PushBack(i1);
                    m_indices.PushBack(i2);
                }
            }
        }

        {
            auto timer = ScopedTimer("Generating Terrain Normals", m_pSystemsManager);
            GeometryUtils::GenerateNormals(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Generating Terrain Tangents", m_pSystemsManager);
            GeometryUtils::GenerateTerrainTangents(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Creating Terrain Geometry", m_pSystemsManager);

            if (!Renderer.CreateGeometry(m_geometry, sizeof(TerrainVertex), m_vertices.Size(), m_vertices.GetData(), sizeof(u32),
                                         m_indices.Size(), m_indices.GetData()))
            {
                ERROR_LOG("Failed to create geometry.");
                return;
            }
        }

        {
            auto timer = ScopedTimer("Uploading Terrain Geometry", m_pSystemsManager);

            if (!Renderer.UploadGeometry(m_geometry))
            {
                ERROR_LOG("Failed to upload geometry.");
                return;
            }
        }

        m_geometry.center  = m_origin;
        m_geometry.extents = m_extents;
        // TODO: This should be done in the renderer (CreateGeometry() method)
        m_geometry.generation++;

        m_id.Generate();

        {
            auto timer = ScopedTimer("Acquiring Terrain Material", m_pSystemsManager);

            String terrainMaterialName = String::FromFormat("terrain_mat_{}", m_name);
            m_geometry.material        = Materials.AcquireTerrain(terrainMaterialName, m_config.materials, true);
            if (!m_geometry.material)
            {
                WARN_LOG("Failed to acquire terrain material. Using default instead.");
                m_geometry.material = Materials.GetDefaultTerrain();
            }
        }

        {
            auto timer = ScopedTimer("Unloading Terrain Config", m_pSystemsManager);

            Resources.Unload(m_config);
        }
    }

    void Terrain::LoadJobFailure()
    {
        ERROR_LOG("Failed to load: '{}'.", m_config.resourceName);

        Resources.Unload(m_config);
    }
}  // namespace C3D