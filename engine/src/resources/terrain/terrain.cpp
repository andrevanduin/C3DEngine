
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
    Terrain::Terrain() : m_logger("TERRAIN") {}

    bool Terrain::Create(const SystemManager* pSystemsManager, const TerrainConfig& config)
    {
        m_pSystemsManager = pSystemsManager;
        m_config          = config;
        m_name            = config.name;
        return true;
    }

    bool Terrain::Initialize()
    {
        m_extents.min = { 0, 0, 0 };
        m_extents.max = { 0, 0, 0 };
        m_origin      = { 0, 0, 0 };

        m_vertices.Reserve(m_vertexCount);
        m_indices.Reserve(m_totalTileCount * 6);
        m_materials.Reserve(m_config.materials.Size());

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

    bool Terrain::Unload() { return true; }

    bool Terrain::Update() { return true; }

    bool Terrain::Render(FrameData& frameData, const mat4& projection, const mat4& view, const mat4& model,
                         const vec4& ambientColor, const vec3& viewPosition, u32 renderMode)
    {
        return true;
    }

    void Terrain::Destroy() {}

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
            auto timer = ScopedTimer("Generating Vertices", m_pSystemsManager);

            if (m_config.tileCountX == 0)
            {
                m_logger.Error("TileCountX must > 0.");
                return;
            }

            if (m_config.tileCountZ == 0)
            {
                m_logger.Error("TileCountZ must be > 0.");
                return;
            }

            if (m_config.tileScaleX <= 0.0f)
            {
                m_logger.Error("TileScaleX must be > 0.");
                return;
            }

            if (m_config.tileScaleZ <= 0.0f)
            {
                m_logger.Error("TileScaleZ must be > 0.");
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

                    vert.position = { x * m_tileScaleX, m_config.vertexConfigs[i].height * m_tileScaleY,
                                      z * m_tileScaleZ };
                    vert.color    = vec4(1.0f);
                    vert.normal   = { 0, 1, 0 };
                    vert.texture  = { x, z };
                    // TODO: Materials!
                    // std::memset(vert.materialWeights, 0, sizeof(f32) * TERRAIN_MAX_MATERIAL_COUNT);
                    // vert.materialWeights[0] = 1.0f;
                    vert.tangent = vec3(0);

                    m_vertices.PushBack(vert);
                }
            }
        }

        {
            auto timer = ScopedTimer("Generating Indices", m_pSystemsManager);

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
            auto timer = ScopedTimer("Generating Normals", m_pSystemsManager);
            GeometryUtils::GenerateNormals(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Generating Tangents", m_pSystemsManager);
            GeometryUtils::GenerateTangents(m_vertices, m_indices);
        }

        {
            auto timer = ScopedTimer("Creating Geometry", m_pSystemsManager);

            if (!Renderer.CreateGeometry(&m_geometry, sizeof(TerrainVertex), m_vertices.Size(), m_vertices.GetData(),
                                         sizeof(u32), m_indices.Size(), m_indices.GetData()))
            {
                m_logger.Error("LoadJobSuccess() - Failed to create geometry.");
                return;
            }
        }

        m_geometry.center  = m_origin;
        m_geometry.extents = m_extents;
        // TODO: This should be done in the renderer (CreateGeometry() method)
        m_geometry.generation++;

        Resources.Unload(m_config);
    }

    void Terrain::LoadJobFailure()
    {
        m_logger.Error("LoadJobFailure() - Failed to load: '{}'", m_config.resourceName);

        Resources.Unload(m_config);
    }
}  // namespace C3D