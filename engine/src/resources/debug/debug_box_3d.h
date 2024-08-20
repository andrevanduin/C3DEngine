
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "core/uuid.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"

namespace C3D
{
    class SystemManager;

    class C3D_API DebugBox3D
    {
    public:
        DebugBox3D() = default;

        bool Create(const vec3& size, Transform* parent);

        void Destroy();

        bool Initialize();
        bool Load();
        bool Unload();

        bool Update();
        void OnPrepareRender(FrameData& frameData);

        void SetParent(Transform* parent);

        void SetPosition(const vec3& position);

        void SetColor(const vec4& color);

        void SetExtents(const Extents3D& extents);

        bool IsValid() const { return m_geometry.generation != INVALID_ID_U16; }

        mat4 GetModel() const { return m_transform.GetWorld(); }

        const Geometry* GetGeometry() const { return &m_geometry; }

        UUID GetId() const { return m_id; }

    private:
        void UpdateVertexColor();

        void RecalculateExtents(const Extents3D& extents);

        UUID m_id;
        String m_name;

        vec3 m_size;
        vec4 m_color;
        Transform m_transform;

        DynamicArray<ColorVertex3D> m_vertices;

        bool m_isDirty = true;

        Geometry m_geometry;
    };
}  // namespace C3D