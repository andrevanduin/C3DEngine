
#include "debug_box_3d.h"

#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"

namespace C3D
{
    bool DebugBox3D::Create(const SystemManager* systemsManager, const vec3& size, Transform* parent)
    {
        m_pSystemsManager = systemsManager;

        // TODO: name?
        m_uuid.Generate();
        m_size  = size;
        m_color = vec4(1.0f);  // Default = white

        m_geometry.id         = INVALID_ID;
        m_geometry.generation = INVALID_ID_U16;
        m_geometry.internalId = INVALID_ID;

        if (parent)
        {
            SetParent(parent);
        }

        return true;
    }

    void DebugBox3D::Destroy()
    {
        m_uuid.Invalidate();
        m_vertices.Destroy();
    }

    bool DebugBox3D::Initialize()
    {
        m_vertices.Resize(2 * 12);  // 12 lines to make a cube

        Extents3D extents = { -m_size * 0.5f, m_size * 0.5f };
        RecalculateExtents(extents);
        UpdateVertexColor();

        return true;
    }

    bool DebugBox3D::Load()
    {
        if (!Renderer.CreateGeometry(m_geometry, sizeof(ColorVertex3D), m_vertices.Size(), m_vertices.GetData(), 0, 0, 0))
        {
            Logger::Error("[DEBUG_BOX_3D] - Load() - Failed to create geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(m_geometry))
        {
            Logger::Error("[DEBUG_BOX_3D] - Load() - Failed to upload geometry.");
            return false;
        }

        m_geometry.generation++;
        return true;
    }

    bool DebugBox3D::Unload()
    {
        Renderer.DestroyGeometry(m_geometry);
        return true;
    }

    bool DebugBox3D::Update() { return true; }

    void DebugBox3D::SetParent(Transform* parent) { m_transform.SetParent(parent); }

    void DebugBox3D::SetPosition(const vec3& position) { m_transform.SetPosition(position); }

    void DebugBox3D::SetColor(const vec4& color)
    {
        m_color = color;
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            UpdateVertexColor();

            Renderer.UpdateGeometryVertices(m_geometry, 0, m_vertices.Size(), m_vertices.GetData());

            m_geometry.generation++;
            // Rollover to zero when we reach INVALID_ID_U16 again for the generation
            if (m_geometry.generation == INVALID_ID_U16)
            {
                m_geometry.generation = 0;
            }
        }
    }

    void DebugBox3D::SetExtents(const Extents3D& extents)
    {
        if (m_geometry.generation != INVALID_ID_U16 && !m_vertices.Empty())
        {
            RecalculateExtents(extents);

            Renderer.UpdateGeometryVertices(m_geometry, 0, m_vertices.Size(), m_vertices.GetData());

            m_geometry.generation++;
            // Rollover to zero when we reach INVALID_ID_U16 again for the generation
            if (m_geometry.generation == INVALID_ID_U16)
            {
                m_geometry.generation = 0;
            }
        }
    }

    void DebugBox3D::UpdateVertexColor()
    {
        for (auto& vert : m_vertices)
        {
            vert.color = m_color;
        }
    }

    void DebugBox3D::RecalculateExtents(const Extents3D& extents)
    {
        // Front lines
        {
            // Top
            m_vertices[0].position = vec4(extents.min.x, extents.min.y, extents.min.z, 1.0f);
            m_vertices[1].position = vec4(extents.max.x, extents.min.y, extents.min.z, 1.0f);
            // Right
            m_vertices[2].position = vec4(extents.max.x, extents.min.y, extents.min.z, 1.0f);
            m_vertices[3].position = vec4(extents.max.x, extents.max.y, extents.min.z, 1.0f);
            // Bottom
            m_vertices[4].position = vec4(extents.max.x, extents.max.y, extents.min.z, 1.0f);
            m_vertices[5].position = vec4(extents.min.x, extents.max.y, extents.min.z, 1.0f);
            // Left
            m_vertices[6].position = vec4(extents.min.x, extents.min.y, extents.min.z, 1.0f);
            m_vertices[7].position = vec4(extents.min.x, extents.max.y, extents.min.z, 1.0f);
        }
        // Back Lines
        {
            // Top
            m_vertices[8].position = vec4(extents.min.x, extents.min.y, extents.max.z, 1.0f);
            m_vertices[9].position = vec4(extents.max.x, extents.min.y, extents.max.z, 1.0f);
            // Right
            m_vertices[10].position = vec4(extents.max.x, extents.min.y, extents.max.z, 1.0f);
            m_vertices[11].position = vec4(extents.max.x, extents.max.y, extents.max.z, 1.0f);
            // Bottom
            m_vertices[12].position = vec4(extents.max.x, extents.max.y, extents.max.z, 1.0f);
            m_vertices[13].position = vec4(extents.min.x, extents.max.y, extents.max.z, 1.0f);
            // Left
            m_vertices[14].position = vec4(extents.min.x, extents.min.y, extents.max.z, 1.0f);
            m_vertices[15].position = vec4(extents.min.x, extents.max.y, extents.max.z, 1.0f);
        }

        // Top Connecting Lines
        {
            // Left
            m_vertices[16].position = vec4(extents.min.x, extents.min.y, extents.min.z, 1.0f);
            m_vertices[17].position = vec4(extents.min.x, extents.min.y, extents.max.z, 1.0f);
            // Right
            m_vertices[18].position = vec4(extents.max.x, extents.min.y, extents.min.z, 1.0f);
            m_vertices[19].position = vec4(extents.max.x, extents.min.y, extents.max.z, 1.0f);
        }
        // Bottom Connecting Lines
        {
            // Left
            m_vertices[20].position = vec4(extents.min.x, extents.max.y, extents.min.z, 1.0f);
            m_vertices[21].position = vec4(extents.min.x, extents.max.y, extents.max.z, 1.0f);
            // Right
            m_vertices[22].position = vec4(extents.max.x, extents.max.y, extents.min.z, 1.0f);
            m_vertices[23].position = vec4(extents.max.x, extents.max.y, extents.max.z, 1.0f);
        }
    }
}  // namespace C3D