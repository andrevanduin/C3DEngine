
#pragma once
#include "defines.h"
#include "identifiers/uuid.h"
#include "renderer/geometry.h"
#include "renderer/vertex.h"
#include "string/string.h"
#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    class C3D_API DebugLine3D
    {
    public:
        DebugLine3D() = default;

        bool Create(const vec3& point0, const vec3& point1);

        void Destroy();

        bool Initialize();
        bool Load();
        bool Unload();

        void OnPrepareRender(FrameData& frameData);
        bool Update();

        UUID GetId() const { return m_id; }
        mat4 GetModel() const { return Transforms.GetWorld(m_transform); }
        const Geometry* GetGeometry() const { return &m_geometry; }

        void SetColor(const vec4& color);
        void SetPoints(const vec3& point0, const vec3& point1);

    private:
        void UpdateVertexColor();
        void RecalculatePoints();

        UUID m_id;
        String m_name;

        vec3 m_point0, m_point1;
        vec4 m_color;

        Handle<Transform> m_transform;

        DynamicArray<ColorVertex3D> m_vertices;

        bool m_isDirty = true;

        Geometry m_geometry;
    };
}  // namespace C3D