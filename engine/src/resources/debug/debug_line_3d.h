
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "renderer/geometry.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"

namespace C3D
{
    class SystemManager;

    class C3D_API DebugLine3D
    {
    public:
        DebugLine3D() = default;

        bool Create(const SystemManager* systemsManager, const vec3& point0, const vec3& point1, Transform* parent);

        void Destroy();

        bool Initialize();
        bool Load();
        bool Unload();

        bool Update();

        mat4 GetModel() const { return m_transform.GetWorld(); }
        const Geometry* GetGeometry() const { return &m_geometry; }

        void SetParent(Transform* parent) { m_transform.SetParent(parent); }
        void SetColor(const vec4& color);
        void SetPoints(const vec3& point0, const vec3& point1);

    private:
        void UpdateVertexColor();
        void RecalculatePoints();

        u32 m_uniqueId = INVALID_ID;
        String m_name;

        vec3 m_point0, m_point1;
        vec4 m_color;

        Transform m_transform;

        DynamicArray<ColorVertex3D> m_vertices;

        Geometry m_geometry;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D