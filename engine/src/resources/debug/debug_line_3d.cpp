
#include "debug_line_3d.h"

#include "core/identifier.h"
#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"

namespace C3D
{
    bool DebugLine3D::Create(const SystemManager* systemsManager, const vec3& point0, const vec3& point1,
                             Transform* parent)
    {
        m_pSystemsManager = systemsManager;

        m_point0   = point0;
        m_point1   = point1;
        m_uniqueId = Identifier::GetNewId(this);
        m_color    = vec4(1.0f);  // Default is white

        m_geometry.id         = INVALID_ID;
        m_geometry.generation = INVALID_ID_U16;
        m_geometry.internalId = INVALID_ID;

        if (parent)
        {
            SetParent(parent);
        }

        return true;
    }

    void DebugLine3D::Destroy()
    {
        Identifier::ReleaseId(m_uniqueId);
        m_uniqueId = INVALID_ID;

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
        if (!Renderer.CreateGeometry(m_geometry, sizeof(ColorVertex3D), m_vertices.Size(), m_vertices.GetData(), 0, 0,
                                     0))
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

    bool DebugLine3D::Update() { return true; }

    void DebugLine3D::SetColor(const vec4& color)
    {
        m_color = color;
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            UpdateVertexColor();

            Renderer.UpdateGeometryVertices(m_geometry, 0, m_vertices.Size(), m_vertices.GetData());

            m_geometry.generation++;

            // Roll over back to zero when our generation increments back to INVALID_ID_U16
            if (m_geometry.generation == INVALID_ID_U16)
            {
                m_geometry.generation = 0;
            }
        }
    }

    void DebugLine3D::SetPoints(const vec3& point0, const vec3& point1)
    {
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            m_point0 = point0;
            m_point1 = point1;

            RecalculatePoints();

            Renderer.UpdateGeometryVertices(m_geometry, 0, m_vertices.Size(), m_vertices.GetData());

            m_geometry.generation++;

            // Roll over back to zero when our generation increments back to INVALID_ID_U16
            if (m_geometry.generation == INVALID_ID_U16)
            {
                m_geometry.generation = 0;
            }
        }
    }

    void DebugLine3D::UpdateVertexColor()
    {
        if (!m_vertices.Empty())
        {
            m_vertices[0].color = m_color;
            m_vertices[1].color = m_color;
        }
    }
    void DebugLine3D::RecalculatePoints()
    {
        if (!m_vertices.Empty())
        {
            m_vertices[0].position = vec4(m_point0, 1.0f);
            m_vertices[1].position = vec4(m_point1, 1.0f);
        }
    }
}  // namespace C3D