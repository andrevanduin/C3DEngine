
#include "audio_system.h"

#include "core/audio/audio_emitter.h"
#include "core/audio/audio_plugin.h"
#include "math/c3d_math.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "AUDIO_SYSTEM";

    AudioSystem::AudioSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool AudioSystem::OnInit(const AudioSystemConfig& config)
    {
        m_config = config;

        if (config.numAudioChannels < 4)
        {
            ERROR_LOG("Number of audio channels should be >= 4.");
            return false;
        }

        if (config.chunkSize == 0)
        {
            ERROR_LOG("Please provided a valid chunk size.");
            return false;
        }

        AudioPluginConfig pluginConfig;
        pluginConfig.maxSources   = m_config.numAudioChannels;
        pluginConfig.maxBuffers   = 256;  // Number of buffers that is guaranteed by spec to exist
        pluginConfig.chunkSize    = m_config.chunkSize;
        pluginConfig.frequency    = m_config.frequency;
        pluginConfig.channelCount = ToUnderlying(m_config.channelType);

        // Load our plugin library
        m_pluginLibrary.Load(m_config.pluginName);
        // Create our plugin
        m_audioPlugin = m_pluginLibrary.CreatePlugin<AudioPlugin>(pluginConfig);
        // Initialize our plugin
        if (!m_audioPlugin->Init(pluginConfig))
        {
            ERROR_LOG("Failed to initialize Audio plugin.");
            return false;
        }

        return true;
    }

    void AudioSystem::OnUpdate(const FrameData& frameData)
    {
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            auto& channel = m_channels[i];
            if (channel.emitter)
            {
                m_audioPlugin->SetSourcePosition(i, channel.emitter->position);
                m_audioPlugin->SetSourceLoop(i, channel.emitter->loop);
                m_audioPlugin->SetSourceGain(i, m_masterVolume * channel.volume * channel.emitter->volume);
            }
        }

        return m_audioPlugin->OnUpdate(frameData);
    }

    bool AudioSystem::SetListenerOrientation(const vec3& position, const vec3& forward, const vec3& up)
    {
        m_audioPlugin->SetListenerPosition(position);
        m_audioPlugin->SetListenerOrientation(forward, up);
    }

    AudioHandle AudioSystem::LoadChunk(const char* name) const { return m_audioPlugin->LoadChunk(name); }

    AudioHandle AudioSystem::LoadStream(const char* name) const { return m_audioPlugin->LoadStream(name); }

    void AudioSystem::Close(const AudioHandle& handle) const { m_audioPlugin->Unload(handle); }

    void AudioSystem::SetMasterVolume(f32 volume)
    {
        m_masterVolume = Clamp(volume, 0.0f, 1.0f);
        // Now adjust each channel's volume to take the new master volume into account
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            f32 mixedVolume = m_channels[i].volume * m_masterVolume;
            if (m_channels[i].emitter)
            {
                // Take the emitter's volume into account if there is one
                mixedVolume *= m_channels[i].emitter->volume;
            }
            m_audioPlugin->SetSourceGain(i, mixedVolume);
        }
    }

    f32 AudioSystem::GetMasterVolume() const { return m_masterVolume; }

    bool AudioSystem::SetChannelVolume(u8 channelIndex, f32 volume)
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
            return false;
        }

        auto& channel  = m_channels[channelIndex];
        channel.volume = Clamp(volume, 0.0f, 1.0f);

        f32 mixedVolume = channel.volume * m_masterVolume;
        if (channel.emitter)
        {
            mixedVolume *= channel.emitter->volume;
        }

        m_audioPlugin->SetSourceGain(channelIndex, mixedVolume);
        return true;
    }

    f32 AudioSystem::GetChannelVolume(u8 channelIndex) const
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
        }
        return m_channels[channelIndex].volume;
    }

    bool AudioSystem::PlayOnChannel(u8 channelIndex, const AudioHandle& handle, bool loop)
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
        }

        auto& channel   = m_channels[channelIndex];
        channel.emitter = nullptr;
        channel.current = handle;

        // Set the channel volume
        m_audioPlugin->SetSourceGain(channelIndex, m_masterVolume * channel.volume);

        if (handle.type == AudioHandleType::SoundEffect)
        {
            vec3 pos = m_audioPlugin->GetListenerPosition();
            m_audioPlugin->SetSourcePosition(channelIndex, pos);
            m_audioPlugin->SetSourceLoop(channelIndex, loop);
        }

        m_audioPlugin->SourceStop(channelIndex);
        return m_audioPlugin->SourcePlay(channelIndex, handle);
    }

    bool AudioSystem::Play(const AudioHandle& handle, bool loop)
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
        }
    }

    bool AudioSystem::PlayEmitterOnChannel(u8 channelIndex, const AudioHandle& handle) {}

    bool AudioSystem::PlayEmitter(const AudioHandle& handle) {}

    void AudioSystem::StopChannel(u8 channelIndex) {}

    void AudioSystem::PauseChannel(u8 channelIndex) {}

    void AudioSystem::ResumeChannel(u8 channelIndex) {}

    void AudioSystem::OnShutdown()
    {
        m_audioPlugin->Shutdown();
        m_pluginLibrary.DeletePlugin(m_audioPlugin);

        if (!m_pluginLibrary.Unload())
        {
            ERROR_LOG("Failed to unload audio plugin dynamic library.");
        }
    }
}  // namespace C3D