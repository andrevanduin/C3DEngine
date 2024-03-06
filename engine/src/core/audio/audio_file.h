
#pragma once
#include "audio_types.h"
#include "core/defines.h"

namespace C3D
{
    class AudioFile
    {
    public:
        virtual u64 LoadSamples(u32 chunkSize, i32 count) = 0;
        virtual void StreamBufferData()                   = 0;
        virtual void Rewind()                             = 0;

    private:
        AudioFileType m_type;

        u32 m_format;
        u32 m_sampleRate;
        u32 m_totalSamplesLeft;

        ChannelType m_channelType;
    };
}  // namespace C3D