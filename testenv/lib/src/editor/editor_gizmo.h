
#pragma once
#include <containers/dynamic_array.h>
#include <identifiers/uuid.h>
#include <logger/logger.h>
#include <math/math_types.h>
#include <math/ray.h>
#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/vertex.h>
#include <systems/system_manager.h>
#include <systems/transforms/transform_system.h>

enum class EditorGizmoMode
{
    None,
    Move,
    Rotate,
    Scale,
    Max,
};

enum class EditorGizmoInteractionType
{
    None,
    MouseHover,
    MouseDown,
    MouseDrag,
    MouseUp,
    Cancel,
    Max,
};

enum class EditorGizmoOrientation
{
    /** @brief The transform operations are relative to the global transform. */
    Global,
    /** @brief The transform operations are relative to the local transform. */
    Local,
    Max,
};

struct EditorGizmoModeData
{
    C3D::DynamicArray<C3D::ColorVertex3D> vertices;
    C3D::DynamicArray<u32> indices;
    C3D::DynamicArray<C3D::Extents3D> extents;

    u8 currentAxisIndex = INVALID_ID_U8;

    C3D::Plane3D interactionPlane, interactionPlaneBack;

    vec3 interactionStartPos;
    vec3 interactionLastPos;

    C3D::Geometry geometry;
};

namespace C3D
{
    class SystemManager;
}

class EditorGizmo
{
public:
    EditorGizmo() = default;

    bool Create();
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    void OnPrepareRender(C3D::FrameData& frameData);

    void Update();
    void Refresh();

    void BeginInteraction(EditorGizmoInteractionType interactionType, const C3D::Camera* camera, const C3D::Ray& ray);
    void HandleInteraction(const C3D::Ray& ray);
    void EndInteraction();

    EditorGizmoInteractionType GetCurrentInteractionType() const { return m_interaction; }
    EditorGizmoOrientation GetOrientation() const { return m_orientation; }

    C3D::UUID GetId() const { return m_id; }
    vec3 GetPosition() const { return Transforms.GetPosition(m_transform); }
    mat4 GetModel() const { return Transforms.GetWorld(m_transform); }
    C3D::Geometry* GetGeometry();

    void SetMode(const EditorGizmoMode mode) { m_mode = mode; }
    void SetScale(const f32 scale) { m_scale = scale; }
    void SetOrientation(const EditorGizmoOrientation orientation);

    void SetSelectedObjectTransform(C3D::Handle<C3D::Transform> selected);

private:
    void CreateNoneMode();
    void CreateMoveMode();
    void CreateScaleMode();
    void CreateRotateMode();

    C3D::Handle<C3D::Transform> m_transform;
    C3D::Handle<C3D::Transform> m_selectedObjectTransform;

    /** @brief Used to keep the gizmo a consistent size on the screen despite camera distance. */
    f32 m_scale = 0.0f;

    C3D::UUID m_id;

    EditorGizmoMode m_mode;
    EditorGizmoModeData m_modeData[ToUnderlying(EditorGizmoMode::Max)];

    EditorGizmoInteractionType m_interaction;
    EditorGizmoOrientation m_orientation;

    bool m_isDirty = true;
};