
#include "debug_grid.h"

#include "core/identifier.h"
#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"

namespace C3D
{
    bool DebugGrid::Create(const SystemManager* pSystemsManager, const DebugGridConfig& config)
    {
        m_pSystemsManager = pSystemsManager;

        m_tileCountDim0 = config.tileCountDim0;
        m_tileCountDim1 = config.tileCountDim1;
        m_tileScale     = config.tileScale;
        m_name          = config.name;
        m_orientation   = config.orientation;
        m_useThirdAxis  = config.useThirdAxis;

        f32 max0 = m_tileCountDim0 * m_tileScale;
        f32 min0 = -max0;
        f32 max1 = m_tileCountDim1 * m_tileScale;
        f32 min1 = -max1;

        m_extents.min = vec3(0);
        m_extents.max = vec3(0);

        switch (m_orientation)
        {
            default:
            case DebugGridOrientation::XZ:
                m_extents.min.x = min0;
                m_extents.max.x = max0;
                m_extents.min.z = min1;
                m_extents.max.z = max1;
                break;
            case DebugGridOrientation::XY:
                m_extents.min.x = min0;
                m_extents.max.x = max0;
                m_extents.min.y = min1;
                m_extents.max.y = max1;
                break;
            case DebugGridOrientation::YZ:
                m_extents.min.y = min0;
                m_extents.max.y = max0;
                m_extents.min.z = min1;
                m_extents.max.z = max1;
                break;
        }

        m_origin   = vec3(0);
        m_uniqueId = Identifier::GetNewId(this);

        // Two vertices per line, one line per tile in each direction, plus one in the middle for each direction.
        // And two extra for the third axis.
        const auto vertexCount =
            (m_tileCountDim0 * 2 + 1) * 2 + (m_tileCountDim1 * 2 + 1) * 2 + (m_useThirdAxis ? 2 : 0);
        m_vertices.Resize(vertexCount);

        return true;
    }

    void DebugGrid::Destroy()
    {
        Identifier::ReleaseId(m_uniqueId);
        m_uniqueId = INVALID_ID;
    }

    bool DebugGrid::Initialize()
    {
        f32 lineLength0 = m_tileCountDim1 * m_tileScale;
        f32 lineLength1 = m_tileCountDim0 * m_tileScale;
        f32 lineLength2 = std::max(lineLength0, lineLength1);

        u32 elementIndex0, elementIndex1, elementIndex2;

        switch (m_orientation)
        {
            default:
            case DebugGridOrientation::XZ:
                elementIndex0 = 0;  // X
                elementIndex1 = 2;  // Z
                elementIndex2 = 1;  // Y
                break;
            case DebugGridOrientation::XY:
                elementIndex0 = 0;  // X
                elementIndex1 = 1;  // Y
                elementIndex2 = 2;  // Z
                break;
            case DebugGridOrientation::YZ:
                elementIndex0 = 1;  // Y
                elementIndex1 = 2;  // Z
                elementIndex2 = 0;  // X
                break;
        }

        // First axis line
        m_vertices[0].position[elementIndex0] = -lineLength1;
        m_vertices[0].position[elementIndex1] = 0;
        m_vertices[1].position[elementIndex0] = lineLength1;
        m_vertices[1].position[elementIndex1] = 0;
        m_vertices[0].color[elementIndex0]    = 1.0f;
        m_vertices[0].color.a                 = 1.0f;
        m_vertices[1].color[elementIndex0]    = 1.0f;
        m_vertices[1].color.a                 = 1.0f;

        // Second axis line
        m_vertices[2].position[elementIndex0] = 0;
        m_vertices[2].position[elementIndex1] = -lineLength0;
        m_vertices[3].position[elementIndex0] = 0;
        m_vertices[3].position[elementIndex1] = lineLength0;
        m_vertices[2].color[elementIndex1]    = 1.0f;
        m_vertices[2].color.a                 = 1.0f;
        m_vertices[3].color[elementIndex1]    = 1.0f;
        m_vertices[3].color.a                 = 1.0f;

        if (m_useThirdAxis)
        {
            // Third axis line
            m_vertices[4].position[elementIndex0] = 0;
            m_vertices[4].position[elementIndex2] = -lineLength2;
            m_vertices[5].position[elementIndex0] = 0;
            m_vertices[5].position[elementIndex2] = lineLength2;
            m_vertices[4].color[elementIndex2]    = 1.0f;
            m_vertices[4].color.a                 = 1.0f;
            m_vertices[5].color[elementIndex2]    = 1.0f;
            m_vertices[5].color.a                 = 1.0f;
        }

        constexpr auto altLineColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);
        // Calculate 4 lines at the time, 2 in each direction with min/max
        f32 j = 1.0;

        u32 startIndex = m_useThirdAxis ? 6 : 4;
        for (u32 i = startIndex; i < m_vertices.Size(); i += 8)
        {
            // First line (max)
            m_vertices[i + 0].position[elementIndex0] = j * m_tileScale;
            m_vertices[i + 0].position[elementIndex1] = lineLength0;
            m_vertices[i + 0].color                   = altLineColor;
            m_vertices[i + 1].position[elementIndex0] = j * m_tileScale;
            m_vertices[i + 1].position[elementIndex1] = -lineLength0;
            m_vertices[i + 1].color                   = altLineColor;

            // Second line (min)
            m_vertices[i + 2].position[elementIndex0] = -j * m_tileScale;
            m_vertices[i + 2].position[elementIndex1] = lineLength0;
            m_vertices[i + 2].color                   = altLineColor;
            m_vertices[i + 3].position[elementIndex0] = -j * m_tileScale;
            m_vertices[i + 3].position[elementIndex1] = -lineLength0;
            m_vertices[i + 3].color                   = altLineColor;

            // Third line (max)
            m_vertices[i + 4].position[elementIndex0] = -lineLength1;
            m_vertices[i + 4].position[elementIndex1] = -j * m_tileScale;
            m_vertices[i + 4].color                   = altLineColor;
            m_vertices[i + 5].position[elementIndex0] = lineLength1;
            m_vertices[i + 5].position[elementIndex1] = -j * m_tileScale;
            m_vertices[i + 5].color                   = altLineColor;

            // Fourth line (min)
            m_vertices[i + 6].position[elementIndex0] = -lineLength1;
            m_vertices[i + 6].position[elementIndex1] = j * m_tileScale;
            m_vertices[i + 6].color                   = altLineColor;
            m_vertices[i + 7].position[elementIndex0] = lineLength1;
            m_vertices[i + 7].position[elementIndex1] = j * m_tileScale;
            m_vertices[i + 7].color                   = altLineColor;

            j++;
        }

        m_geometry.internalId = INVALID_ID;
        return true;
    }

    bool DebugGrid::Load()
    {
        if (!Renderer.CreateGeometry(m_geometry, sizeof(ColorVertex3D), m_vertices.Size(), m_vertices.GetData(), 0, 0,
                                     0))
        {
            Logger::Error("[DEBUG_GRID] - Load() - Failed to create geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(m_geometry))
        {
            Logger::Error("[DEBUG_GRID] - Load() - Failed to upload geometry.");
            return false;
        }

        return true;
    }

    bool DebugGrid::Unload()
    {
        if (m_uniqueId != INVALID_ID)
        {
            Renderer.DestroyGeometry(m_geometry);
        }

        return true;
    }

    bool DebugGrid::Update() { return true; }

}  // namespace C3D