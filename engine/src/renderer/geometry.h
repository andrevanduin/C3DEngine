
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
        u16 generation = INVALID_ID_U16;

        vec3 center;
        Extents3D extents;

        u32 vertexCount        = 0;
        u32 vertexSize         = 0;
        void* vertices         = nullptr;
        u64 vertexBufferOffset = 0;

        u32 indexCount        = 0;
        u32 indexSize         = 0;
        void* indices         = nullptr;
        u64 indexBufferOffset = 0;

        String name;
        Material* material = nullptr;
    };
}  // namespace C3D