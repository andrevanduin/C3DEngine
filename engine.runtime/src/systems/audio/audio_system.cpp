
#include "audio_system.h"

#include "audio/audio_plugin.h"
#include "math/c3d_math.h"
#include "resources/managers/audio_manager.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
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

        m_emitters.Create();

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

        // Ensure there is enough room for audio files
        m_audioFiles.Create();

        return true;
    }

    bool AudioSystem::OnUpdate(const FrameData& frameData)
    {
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            auto& channel = m_channels[i];
            if (channel.emitter)
            {
                auto& emitter = m_emitters[channel.emitter];

                m_audioPlugin->SetSourcePosition(i, emitter.position);
                m_audioPlugin->SetSourceLoop(i, emitter.loop);
                m_audioPlugin->SetSourceGain(i, m_masterVolume * channel.volume * emitter.volume);
            }
        }

        return m_audioPlugin->OnUpdate(frameData);
    }

    void AudioSystem::SetListenerOrientation(const vec3& position, const vec3& forward, const vec3& up) const
    {
        m_audioPlugin->SetListenerPosition(position);
        m_audioPlugin->SetListenerOrientation(forward, up);
    }

    AudioHandle AudioSystem::LoadChunk(const char* name)
    {
        AudioFile file;
        AudioFileParams params;
        params.type      = AudioType::SoundEffect;
        params.chunkSize = m_config.chunkSize;

        if (!Resources.Read(name, file, params))
        {
            ERROR_LOG("Failed to load file: '{}'.", name);
        }

        if (!m_audioPlugin->LoadChunk(file))
        {
            ERROR_LOG("Loading chunk failed in the audio backend plugin for: '{}'.", name);
            return AudioHandle::Invalid();
        }

        // Create a handle to this file and save the file off into our internal hashmap
        auto handle = AudioHandle(AudioType::SoundEffect);
        m_audioFiles.Set(handle.uuid, file);

        return handle;
    }

    AudioHandle AudioSystem::LoadStream(const char* name)
    {
        AudioFile file;
        AudioFileParams params;
        params.type      = AudioType::MusicStream;
        params.chunkSize = m_config.chunkSize;

        if (!Resources.Read(name, file, params))
        {
            ERROR_LOG("Failed to load file: '{}'.", name);
        }

        if (!m_audioPlugin->LoadStream(file))
        {
            ERROR_LOG("Loading stream failed in the audio backend plugin for: '{}'.", name);
            return AudioHandle::Invalid();
        }

        // Create a handle to this file and save the file off into our internal hashmap
        auto handle = AudioHandle(AudioType::MusicStream);
        m_audioFiles.Set(handle.uuid, file);

        return handle;
    }

    void AudioSystem::Close(const AudioHandle& handle)
    {
        if (!m_audioFiles.Has(handle.uuid))
        {
            WARN_LOG("Tried to close an unknown AudioHandle: '{}'.", handle.uuid);
            return;
        }

        // Get the audio file
        auto& audio = m_audioFiles.Get(handle.uuid);
        // Close the file
        Close(audio);
        // Remove it from our known audio files
        m_audioFiles.Delete(handle.uuid);
    }

    void AudioSystem::SetMasterVolume(f32 volume)
    {
        m_masterVolume = Clamp(volume, 0.0f, 1.0f);
        // Now adjust each channel's volume to take the new master volume into account
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            auto& channel = m_channels[i];

            f32 mixedVolume = channel.volume * m_masterVolume;
            if (channel.emitter)
            {
                // Take the emitter's volume into account if there is one
                auto& emitter = m_emitters[channel.emitter];
                mixedVolume *= emitter.volume;
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
            // Take the emitter's volume into account if there is one
            auto& emitter = m_emitters[channel.emitter];
            mixedVolume *= emitter.volume;
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
            return false;
        }

        if (!m_audioFiles.Has(handle.uuid))
        {
            ERROR_LOG("Provided AudioHandle: {} is unknown.", handle.uuid);
            return false;
        }

        auto& channel = m_channels[channelIndex];
        channel.emitter.Invalidate();

        // Set the channel volume
        m_audioPlugin->SetSourceGain(channelIndex, m_masterVolume * channel.volume);

        if (handle.type == AudioType::SoundEffect)
        {
            vec3 pos = m_audioPlugin->GetListenerPosition();
            m_audioPlugin->SetSourcePosition(channelIndex, pos);
            m_audioPlugin->SetSourceLoop(channelIndex, loop);
        }

        m_audioPlugin->SourceStop(channelIndex);

        auto& audio = m_audioFiles.Get(handle.uuid);

        channel.current = &audio;
        return m_audioPlugin->SourcePlay(channelIndex, audio);
    }

    bool AudioSystem::Play(const AudioHandle& handle, bool loop)
    {
        i8 channelIndex = -1;

        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            auto& channel = m_channels[i];
            if (!channel.current && !channel.emitter)
            {
                channelIndex = i;
                break;
            }
        }

        if (channelIndex == -1)
        {
            // All channels are playing something. Drop the sound.
            WARN_LOG("No channel available for playing. Dropping this audio.");
            return false;
        }

        return PlayOnChannel(channelIndex, handle, loop);
    }

    bool AudioSystem::PlayEmitterOnChannel(u8 channelIndex, EmitterHandle handle)
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
            return false;
        }

        if (!m_emitters.Has(handle))
        {
            ERROR_LOG("Provided EmitterHandle: {} is unknown.", handle);
            return false;
        }

        auto& emitter = m_emitters.Get(handle);
        auto& channel = m_channels[channelIndex];

        channel.emitter = handle;
        channel.current = &emitter.audio;

        return m_audioPlugin->SourcePlay(channelIndex, emitter.audio);
    }

    bool AudioSystem::PlayEmitter(EmitterHandle handle)
    {
        i8 channelIndex = -1;

        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            auto& channel = m_channels[i];
            if (!channel.current && !channel.emitter)
            {
                channelIndex = i;
                break;
            }
        }

        if (channelIndex == -1)
        {
            // All channels are playing something. Drop the sound.
            WARN_LOG("No channel available for playing. Dropping this audio.");
            return false;
        }

        return PlayEmitterOnChannel(channelIndex, handle);
    }

    void AudioSystem::StopChannel(u8 channelIndex) const
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
            return;
        }
        m_audioPlugin->SourceStop(channelIndex);
    }

    void AudioSystem::StopAllChannels() const
    {
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            m_audioPlugin->SourceStop(i);
        }
    }

    void AudioSystem::PauseChannel(u8 channelIndex) const
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
            return;
        }
        m_audioPlugin->SourcePause(channelIndex);
    }

    void AudioSystem::PauseAllChannels() const
    {
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            m_audioPlugin->SourceStop(i);
        }
    }

    void AudioSystem::ResumeChannel(u8 channelIndex) const
    {
        if (channelIndex >= m_config.numAudioChannels)
        {
            ERROR_LOG("Channel index: {} >= the number of available channels ({}).", channelIndex, m_config.numAudioChannels);
            return;
        }
        m_audioPlugin->SourceResume(channelIndex);
    }

    void AudioSystem::ResumeAllChannels() const
    {
        for (u32 i = 0; i < m_config.numAudioChannels; i++)
        {
            m_audioPlugin->SourceResume(i);
        }
    }

    void AudioSystem::Close(AudioFile& file)
    {
        // Unload it in the plugin
        m_audioPlugin->Unload(file);
        // Unload the resource
        Resources.Cleanup(file);
    }

    void AudioSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");

        m_emitters.Destroy();

        INFO_LOG("Unloading all Audio Files");
        for (auto& file : m_audioFiles)
        {
            Close(file);
        }
        m_audioFiles.Destroy();

        m_audioPlugin->Shutdown();
        m_pluginLibrary.DeletePlugin(m_audioPlugin);

        if (!m_pluginLibrary.Unload())
        {
            ERROR_LOG("Failed to unload audio plugin dynamic library.");
        }
    }
}  // namespace C3D