
#pragma once
#include "core/audio/audio_types.h"
#include "core/defines.h"

namespace C3D
{
    struct FrameData;

    struct AudioPluginConfig
    {
        u32 maxSources;
        u32 maxBuffers;
        u32 chunkSize;
        u32 frequency;
        u8 channelCount;
    };

    class AudioPlugin
    {
    public:
        virtual bool Init(const AudioPluginConfig& config) = 0;
        virtual void Shutdown()                            = 0;
        virtual void OnUpdate(const FrameData& frameData)  = 0;

        virtual vec3 GetListenerPosition() = 0;

        virtual void SetSourcePosition(u8 channelIndex, const vec3& position) = 0;
        virtual void SetSourceLoop(u8 channelIndex, bool loop)                = 0;
        virtual void SetSourceGain(u8 channelIndex, f32 gain)                 = 0;

        virtual void SetListenerPosition(const vec3& position)                   = 0;
        virtual void SetListenerOrientation(const vec3& forward, const vec3& up) = 0;

        virtual AudioHandle LoadChunk(const char* name)  = 0;
        virtual AudioHandle LoadStream(const char* name) = 0;

        virtual void SourceStop(u8 channelId)                            = 0;
        virtual bool SourcePlay(u8 channelId, const AudioHandle& handle) = 0;

        virtual void Unload(AudioHandle handle) = 0;

    protected:
        AudioPluginConfig m_config;
    };
}  // namespace C3D