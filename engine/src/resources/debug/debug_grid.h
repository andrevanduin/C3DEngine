
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "renderer/geometry.h"
#include "renderer/vertex.h"

namespace C3D
{
    class SystemManager;

    enum class DebugGridOrientation
    {
        XZ = 0,
        XY = 1,
        YZ = 2
    };

    struct DebugGridConfig
    {
        String name;
        DebugGridOrientation orientation;

        /** @brief The tile count in the first and second dimension (as specified by the orientation) from both
         * directions outwards from the origin.
         */
        u32 tileCountDim0, tileCountDim1;
        /** @brief The scale of the tiles in both axes. Relative to one unit. */
        f32 tileScale = 1.0f;

        /** @brief Bool indicating if we should also draw a third axis (x, y or z depending on config). */
        bool useThirdAxis;
    };

    class C3D_API DebugGrid
    {
    public:
        DebugGrid() = default;

        bool Create(const SystemManager* pSystemsManager, const DebugGridConfig& config);

        void Destroy();

        bool Initialize();
        bool Load();
        bool Unload();
        bool Update();

        Geometry* GetGeometry() { return &m_geometry; }

    private:
        u32 m_uniqueId = INVALID_ID;
        String m_name;

        DebugGridOrientation m_orientation;

        u32 m_tileCountDim0 = 0, m_tileCountDim1 = 0;
        f32 m_tileScale     = 1.0f;
        bool m_useThirdAxis = false;

        Extents3D m_extents;
        vec3 m_origin;

        DynamicArray<ColorVertex3D> m_vertices;

        Geometry m_geometry;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D