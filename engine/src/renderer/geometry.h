
#pragma once
#include "containers/string.h"
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
    struct Material;

    struct Geometry
    {
        u32 id         = INVALID_ID;
        u32 internalId = INVALID_ID;
        u16 generation = INVALID_ID_U16;

        vec3 center;
        Extents3D extents;

        u32 vertexCount       = 0;
        u32 vertexElementSize = 0;
        void* vertices        = nullptr;

        u32 indexCount       = 0;
        u32 indexElementSize = 0;
        void* indices        = nullptr;

        String name;
        Material* material = nullptr;
    };
}  // namespace C3D