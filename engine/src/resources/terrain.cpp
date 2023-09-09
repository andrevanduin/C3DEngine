
#include "terrain.h"

#include "renderer/renderer_frontend.h"
#include "systems/lights/light_system.h"
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
        if (m_config.tileCountX == 0)
        {
            m_logger.Error("TileCountX must > 0.");
            return false;
        }

        if (m_config.tileCountZ == 0)
        {
            m_logger.Error("TileCountZ must be > 0.");
            return false;
        }

        if (m_config.tileScaleX <= 0.0f)
        {
            m_logger.Error("TileScaleX must be > 0.");
            return false;
        }

        if (m_config.tileScaleZ <= 0.0f)
        {
            m_logger.Error("TileScaleZ must be > 0.");
            return false;
        }

        m_tileCountX = m_config.tileCountX;
        m_tileCountZ = m_config.tileCountZ;

        m_tileScaleX = m_config.tileScaleX;
        m_tileScaleZ = m_config.tileScaleZ;

        m_totalTileCount = m_tileCountX * m_tileCountZ;
        m_vertexCount    = m_totalTileCount;

        m_extents.min = { 0, 0, 0 };
        m_extents.max = { 0, 0, 0 };
        m_origin      = { 0, 0, 0 };

        m_vertices.Reserve(m_vertexCount);
        m_indices.Reserve(m_totalTileCount * 6);
        m_materials.Reserve(m_config.materials.Size());

        // Generate our vertices
        for (u32 z = 0; z < m_tileCountZ; z++)
        {
            for (u32 x = 0; x < m_tileCountX; x++)
            {
                TerrainVertex vert = {};

                vert.position = { x * m_tileScaleX, 0.0f, z * m_tileScaleZ };  // Y will be modified by a heightmap
                vert.color    = vec4(1.0f);
                // TODO: Generate proper normals based on geometry
                vert.normal  = { 0, 1, 0 };
                vert.texture = { x, z };
                // TODO: Materials!
                // std::memset(vert.materialWeights, 0, sizeof(f32) * TERRAIN_MAX_MATERIAL_COUNT);
                // vert.materialWeights[0] = 1.0f;
                // TODO: Generate proper tangents
                vert.tangent = vec3(0);

                m_vertices.PushBack(vert);
            }
        }

        // Generate our indices
        for (u32 z = 0; z < m_tileCountZ - 1; z++)
        {
            for (u32 x = 0; x < m_tileCountX - 1; x++)
            {
                u32 i0 = (z * m_tileCountX) + x;
                u32 i1 = (z * m_tileCountX) + x + 1;
                u32 i2 = ((z + 1) * m_tileCountX) + x;
                u32 i3 = ((z + 1) * m_tileCountX) + x + 1;

                m_indices.PushBack(i0);
                m_indices.PushBack(i1);
                m_indices.PushBack(i2);
                m_indices.PushBack(i2);
                m_indices.PushBack(i1);
                m_indices.PushBack(i3);
            }
        }
        return true;
    }

    bool Terrain::Load()
    {
        if (!Renderer.CreateGeometry(&m_geometry, sizeof(TerrainVertex), m_vertices.Size(), m_vertices.GetData(),
                                     sizeof(u32), m_indices.Size(), m_indices.GetData()))
        {
            m_logger.Error("Load() - Failed to create geometry.");
            return false;
        }

        m_geometry.center  = m_origin;
        m_geometry.extents = m_extents;
        m_geometry.generation++;

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
}  // namespace C3D