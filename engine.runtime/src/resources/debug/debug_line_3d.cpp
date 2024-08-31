
#include "debug_line_3d.h"

#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"

namespace C3D
{
    bool DebugLine3D::Create(const vec3& point0, const vec3& point1)
    {
        m_point0 = point0;
        m_point1 = point1;
        m_id.Generate();
        m_color     = vec4(1.0f);  // Default is white
        m_transform = Transforms.Acquire();

        m_geometry.id         = INVALID_ID;
        m_geometry.generation = INVALID_ID_U16;

        return true;
    }

    void DebugLine3D::Destroy()
    {
        m_id.Invalidate();
        m_vertices.Destroy();
    }

    bool DebugLine3D::Initialize()
    {
        m_vertices.Resize(2);

        RecalculatePoints();
        UpdateVertexColor();

        return true;
    }

    bool DebugLine3D::Load()
    {
        if (!Renderer.CreateGeometry(m_geometry, sizeof(ColorVertex3D), m_vertices.Size(), m_vertices.GetData(), 0, 0, 0))
        {
            Logger::Error("[DEBUG_LINE] - Load() - Failed to create geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(m_geometry))
        {
            Logger::Error("[DEBUG_LINE] - Load() - Failed to upload geometry.");
            return false;
        }

        m_geometry.generation++;
        return true;
    }

    bool DebugLine3D::Unload()
    {
        Renderer.DestroyGeometry(m_geometry);
        return true;
    }

    void DebugLine3D::OnPrepareRender(FrameData& frameData)
    {
        if (m_isDirty)
        {
            Renderer.UpdateGeometryVertices(m_geometry, 0, m_vertices.Size(), m_vertices.GetData(), false);

            m_geometry.generation++;

            // Roll over back to zero when our generation increments back to INVALID_ID_U16
            if (m_geometry.generation == INVALID_ID_U16)
            {
                m_geometry.generation = 0;
            }

            m_isDirty = false;
        }
    }

    bool DebugLine3D::Update() { return true; }

    void DebugLine3D::SetColor(const vec4& color)
    {
        m_color = color;
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            UpdateVertexColor();
        }
    }

    void DebugLine3D::SetPoints(const vec3& point0, const vec3& point1)
    {
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            m_point0 = point0;
            m_point1 = point1;

            RecalculatePoints();
        }
    }

    void DebugLine3D::UpdateVertexColor()
    {
        if (!m_vertices.Empty())
        {
            m_vertices[0].color = m_color;
            m_vertices[1].color = m_color;
            m_isDirty           = true;
        }
    }
    void DebugLine3D::RecalculatePoints()
    {
        if (!m_vertices.Empty())
        {
            m_vertices[0].position = vec4(m_point0, 1.0f);
            m_vertices[1].position = vec4(m_point1, 1.0f);
            m_isDirty              = true;
        }
    }
}  // namespace C3D