
#pragma once
#include <containers/dynamic_array.h>
#include <core/logger.h>
#include <renderer/geometry.h>
#include <renderer/transform.h>
#include <renderer/vertex.h>

enum class EditorGizmoMode
{
    None,
    Move,
    Rotate,
    Scale,
    Max,
};

struct EditorGizmoModeData
{
    C3D::DynamicArray<C3D::ColorVertex3D> vertices;
    C3D::DynamicArray<u32> indices;

    C3D::Geometry geometry;
};

namespace C3D
{
    class SystemManager;
}

class EditorGizmo
{
public:
    EditorGizmo();

    bool Create(const C3D::SystemManager* pSystemsManager);
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    void Update();

    void SetMode(const EditorGizmoMode mode) { m_mode = mode; }

    C3D::Geometry* GetGeometry();
    vec3 GetPosition() const;
    mat4 GetModel() const;

private:
    void CreateNoneMode();
    void CreateMoveMode();
    void CreateScaleMode();
    void CreateRoateMode();

    C3D::LoggerInstance<16> m_logger;

    C3D::Transform m_transform;

    EditorGizmoMode m_mode;
    EditorGizmoModeData m_modeData[ToUnderlying(EditorGizmoMode::Max)];

    const C3D::SystemManager* m_pSystemsManager;
};