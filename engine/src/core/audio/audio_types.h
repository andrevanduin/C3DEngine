
#pragma once
#include "core/uuid.h"

namespace C3D
{
    enum class AudioPluginType
    {
        None,
        OpenAL
    };

    enum class ChannelType : u8
    {
        Mono   = 1,
        Stereo = 2
    };

    enum class AudioHandleType
    {
        SoundEffect,
        MusicStream
    };

    struct AudioHandle
    {
        AudioHandleType type;
        UUID uuid;
    };
}  // namespace C3D