
#pragma once
#include "defines.h"
#include "identifiers/uuid.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/vertex.h"
#include "string/string.h"
#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    class C3D_API DebugBox3D
    {
    public:
        DebugBox3D() = default;

        bool Create(const vec3& size);

        void Destroy();

        bool Initialize();
        bool Load();
        bool Unload();

        bool Update();
        void OnPrepareRender(FrameData& frameData);

        void SetPosition(const vec3& position);

        void SetColor(const vec4& color);

        void SetExtents(const Extents3D& extents);

        bool IsValid() const { return m_geometry.generation != INVALID_ID_U16; }

        mat4 GetModel() const;

        const Geometry* GetGeometry() const { return &m_geometry; }

        UUID GetId() const { return m_id; }

    private:
        void UpdateVertexColor();

        void RecalculateExtents(const Extents3D& extents);

        UUID m_id;
        String m_name;

        vec3 m_size;
        vec4 m_color;
        Handle<Transform> m_transform;

        DynamicArray<ColorVertex3D> m_vertices;

        bool m_isDirty = true;

        Geometry m_geometry;
    };
}  // namespace C3D