
#pragma once
#include "audio_file.h"
#include "core/defines.h"

namespace C3D
{
    struct AudioEmitter
    {
        vec3 position;

        f32 volume;
        f32 falloff;
        u32 sourceId;

        bool loop;

        AudioFile audio;
    };
}  // namespace C3D