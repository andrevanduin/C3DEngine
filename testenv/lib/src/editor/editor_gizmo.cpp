
#include "editor_gizmo.h"

#include <core/colors.h>
#include <renderer/renderer_frontend.h>
#include <systems/system_manager.h>

EditorGizmo::EditorGizmo() : m_logger("EDITOR_GIZMO") {}

bool EditorGizmo::Create(const C3D::SystemManager* pSystemsManager)
{
    m_pSystemsManager = pSystemsManager;

    m_mode = EditorGizmoMode::None;
    return true;
}

void EditorGizmo::Destroy()
{
    for (auto& data : m_modeData)
    {
        data.vertices.Destroy();
        data.indices.Destroy();
    }
}

bool EditorGizmo::Initialize()
{
    CreateNoneMode();
    CreateMoveMode();
    CreateScaleMode();
    CreateRoateMode();
    return true;
}

bool EditorGizmo::Load()
{
    for (auto& data : m_modeData)
    {
        if (!Renderer.CreateGeometry(data.geometry, sizeof(C3D::ColorVertex3D), data.vertices.Size(),
                                     data.vertices.GetData(), 0, 0, 0))
        {
            m_logger.Error("Load() - Failed to create gizmo geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(data.geometry))
        {
            m_logger.Error("Load() - Failed to upload gizmo geometry.");
            return false;
        }

        data.geometry.generation++;
    }

    return true;
}

bool EditorGizmo::Unload()
{
    for (auto& data : m_modeData)
    {
        Renderer.DestroyGeometry(data.geometry);
    }
    return true;
}

void EditorGizmo::Update() {}

void EditorGizmo::CreateNoneMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::None)];
    data.vertices.Resize(6);

    // X
    data.vertices[0].color      = C3D::GRAY;
    data.vertices[1].color      = C3D::GRAY;
    data.vertices[1].position.x = 1.0f;

    // Y
    data.vertices[2].color      = C3D::GRAY;
    data.vertices[3].color      = C3D::GRAY;
    data.vertices[3].position.y = 1.0f;
    // Z
    data.vertices[4].color      = C3D::GRAY;
    data.vertices[5].color      = C3D::GRAY;
    data.vertices[5].position.z = 1.0f;
}
void EditorGizmo::CreateMoveMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Move)];
    data.vertices.Resize(18);

    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[0].position.x = 0.2f;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 2.0f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[2].position.y = 0.2f;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 2.0f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[4].position.z = 0.2f;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 2.0f;

    // X Box lines
    data.vertices[6].color      = C3D::RED;
    data.vertices[6].position.x = 0.4f;
    data.vertices[7].color      = C3D::RED;
    data.vertices[7].position.x = 0.4f;
    data.vertices[7].position.y = 0.4f;

    data.vertices[8].color      = C3D::RED;
    data.vertices[8].position.x = 0.4f;
    data.vertices[9].color      = C3D::RED;
    data.vertices[9].position.x = 0.4f;
    data.vertices[9].position.z = 0.4f;

    // Y Box lines
    data.vertices[10].color      = C3D::GREEN;
    data.vertices[10].position.y = 0.4f;
    data.vertices[11].color      = C3D::GREEN;
    data.vertices[11].position.y = 0.4f;
    data.vertices[11].position.z = 0.4f;

    data.vertices[12].color      = C3D::GREEN;
    data.vertices[12].position.y = 0.4f;
    data.vertices[13].color      = C3D::GREEN;
    data.vertices[13].position.y = 0.4f;
    data.vertices[13].position.x = 0.4f;

    // Z Box lines
    data.vertices[14].color      = C3D::BLUE;
    data.vertices[14].position.z = 0.4f;
    data.vertices[15].color      = C3D::BLUE;
    data.vertices[15].position.z = 0.4f;
    data.vertices[15].position.y = 0.4f;

    data.vertices[16].color      = C3D::BLUE;
    data.vertices[16].position.z = 0.4f;
    data.vertices[17].color      = C3D::BLUE;
    data.vertices[17].position.z = 0.4f;
    data.vertices[17].position.x = 0.4f;
}

void EditorGizmo::CreateScaleMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Scale)];
    data.vertices.Resize(12);

    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 2.0f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 2.0f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 2.0f;

    // X/Y outer line
    data.vertices[6].position.x = 0.8f;
    data.vertices[6].color      = C3D::RED;
    data.vertices[7].position.y = 0.8f;
    data.vertices[7].color      = C3D::GREEN;

    // Z/Y outer line
    data.vertices[8].position.z = 0.8f;
    data.vertices[8].color      = C3D::BLUE;
    data.vertices[9].position.y = 0.8f;
    data.vertices[9].color      = C3D::GREEN;

    // X/Z outer line
    data.vertices[10].position.x = 0.8f;
    data.vertices[10].color      = C3D::RED;
    data.vertices[11].position.z = 0.8f;
    data.vertices[11].color      = C3D::BLUE;
}

void EditorGizmo::CreateRoateMode()
{
    constexpr u8 segments = 32;
    constexpr f32 radius  = 1.0f;

    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Rotate)];
    data.vertices.Resize(12 + (segments * 2 * 3));

    // Start with the center where we draw small axis
    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 0.2f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 0.2f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 0.2f;

    // Starting index
    u32 j = 6;

    // z
    for (u32 i = 0; i < segments; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / segments * C3D::PI_2;
        data.vertices[j].position.x = radius * C3D::Cos(theta);
        data.vertices[j].position.y = radius * C3D::Sin(theta);
        data.vertices[j].color      = C3D::BLUE;

        theta                           = (f32)((i + 1) % segments) / segments * C3D::PI_2;
        data.vertices[j + 1].position.x = radius * C3D::Cos(theta);
        data.vertices[j + 1].position.y = radius * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::BLUE;
    }

    // y
    for (u32 i = 0; i < segments; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / segments * C3D::PI_2;
        data.vertices[j].position.x = radius * C3D::Cos(theta);
        data.vertices[j].position.z = radius * C3D::Sin(theta);
        data.vertices[j].color      = C3D::GREEN;

        theta                           = (f32)((i + 1) % segments) / segments * C3D::PI_2;
        data.vertices[j + 1].position.x = radius * C3D::Cos(theta);
        data.vertices[j + 1].position.z = radius * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::GREEN;
    }

    // x
    for (u32 i = 0; i < segments; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / segments * C3D::PI_2;
        data.vertices[j].position.y = radius * C3D::Cos(theta);
        data.vertices[j].position.z = radius * C3D::Sin(theta);
        data.vertices[j].color      = C3D::RED;

        theta                           = (f32)((i + 1) % segments) / segments * C3D::PI_2;
        data.vertices[j + 1].position.y = radius * C3D::Cos(theta);
        data.vertices[j + 1].position.z = radius * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::RED;
    }
}

C3D::Geometry* EditorGizmo::GetGeometry() { return &m_modeData[ToUnderlying(m_mode)].geometry; }

vec3 EditorGizmo::GetPosition() const { return m_transform.GetPosition(); }

mat4 EditorGizmo::GetModel() const { return m_transform.GetWorld(); }