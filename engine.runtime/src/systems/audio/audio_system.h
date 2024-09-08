
#pragma once
#include "audio/audio_emitter.h"
#include "audio/audio_file.h"
#include "audio/audio_types.h"
#include "containers/hash_map.h"
#include "dynamic_library/dynamic_library.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    class AudioPlugin;

    namespace
    {
        struct AudioChannel
        {
            f32 volume = 1.0f;
            AudioFile* current;
            EmitterHandle emitter;
        };
    }  // namespace

    constexpr auto MAX_AUDIO_CHANNELS = 16;

    struct AudioSystemConfig
    {
        /** @brief Which plugin should be used under the hood to play audio. */
        String pluginName;
        /** @brief The frequency to output audio at. */
        u32 frequency;
        /** @brief The type of audio channel to use (mono vs stereo). */
        ChannelType channelType;
        /** @brief The size to chunk streamed audio data in. */
        u32 chunkSize;
        /** @brief The number of separately controlled channels used for mixing purposes.
         * For each channel you can individually control it's volume.
         */
        u32 numAudioChannels = MAX_AUDIO_CHANNELS;
    };

    class C3D_API AudioSystem final : public SystemWithConfig<AudioSystemConfig>
    {
    public:
        bool OnInit(const CSONObject& config) override;
        void OnShutdown() override;

        bool OnUpdate(const FrameData& frameData) override;

        /**
         * @brief Sets the orientation of the current listener. Usually this will be the current camera in the world.
         * This can be used with spatial emitters to hear further away sounds at a lower volume etc.
         * @param position The position of the listener in the world
         * @param forward The forward vector of the listener
         * @param up The up vector of the listener
         */
        void SetListenerOrientation(const vec3& position, const vec3& forward, const vec3& up) const;

        /**
         * @brief Loads an audio chunk with the provided name.
         * This loads the entire audio fragment at once so it should only be used for small chunks.
         * @param name The name of the audio chunk you want to load
         * @return A handle to the loaded audio chunk
         */
        AudioHandle LoadChunk(const char* name);

        /**
         * @brief Loads an audio stream with the provided name.
         * This streams the audio making it efficient for large audio fragments
         * @param name The name of the audio stream you want to load
         * @return A handle to the loaded audio stream
         */
        AudioHandle LoadStream(const char* name);

        /**
         * @brief Closes the provided audio handle. This frees all it's internal resources.
         * @param handle To the audio chunk/stream you want to close
         */
        void Close(const AudioHandle& handle);

        /**
         * @brief Set the Master Volume. This will effect all channels equally.
         * Meaning that if you set master volume to 50% all channels will play at 50% of their set volume
         * @param volume The volume to set in a range of [0.0 - 1.0]
         */
        void SetMasterVolume(f32 volume);

        /**
         * @brief Gets the Master Volume.
         * @return f32 The Master Volume
         */
        f32 GetMasterVolume() const;

        /**
         * @brief Set the volume for a particular channel.
         * @param channelIndex The channel you want to set the volume for
         * @param volume The volume you want it set to in a range of [0.0 - 1.0]
         * @return True if successful, otherwise false
         */
        bool SetChannelVolume(u8 channelIndex, f32 volume);

        /**
         * @brief Gets the volume at the provided channel.
         * @param channelIndex The channel you want to get the volume for
         * @return f32 The volume at that particular channel
         */
        f32 GetChannelVolume(u8 channelIndex) const;

        /**
         * @brief Plays the provided sound on the provided channel. This plays the sound globally (without any spatial effects.)
         * @param channelIndex The channel you want the sound to play on
         * @param handle The handle to the audio that you want to play
         * @param loop A boolean indicating if this audio should loop
         * @return True if successful, otherwise false
         */
        bool PlayOnChannel(u8 channelIndex, const AudioHandle& handle, bool loop);

        /**
         * @brief Plays the provided sound on the first freely available channel.
         * This plays the sound globally (without any spatial effects).
         * @param handle The handle to the audio that you want to play
         * @param loop A boolean indicating if this audio should loop
         * @return True if successful, otherwise false
         */
        bool Play(const AudioHandle& handle, bool loop);

        /**
         * @brief Plays the provided emitter on the provided channel. This plays the emitter spatially in the world.
         * @param channelIndex The channel you want the sound to play on
         * @param handle The handle to the audio emitter that you want to play
         * @return True if successful, otherwise false
         */
        bool PlayEmitterOnChannel(u8 channelIndex, EmitterHandle handle);

        /**
         * @brief Plays the provided emitter on the first freely available channel. This plays the emitter spatially in the world.
         * @param handle The handle to the audio you want to play on the emitter
         * @return True if successful, otherwise false
         */
        bool PlayEmitter(EmitterHandle handle);

        /** @brief Stop the audio that is playing on the provided channel.
         * This unassigns the audio from the channel and rewinds it.
         * @param channelIndex The channel you want to stop.
         */
        void StopChannel(u8 channelIndex) const;

        /** @brief Stops the audio on all channels. */
        void StopAllChannels() const;

        /** @brief Pause the audio that is playing on the provided channel.
         * This leaves the audio assigned to this channel and pauses it.
         * @param channelIndex The channel you want to pause.
         */
        void PauseChannel(u8 channelIndex) const;

        /** @brief Pauses the audio on all channels. */
        void PauseAllChannels() const;

        /** @brief Resume the audio that is playing on the provided channel.
         * @param channelIndex The channel you want to resume.
         */
        void ResumeChannel(u8 channelIndex) const;

        /** @brief Resumes the audio on all channels. */
        void ResumeAllChannels() const;

    private:
        /**
         * @brief Closes the provided audio file. This frees all it's internal resources.
         * @param file The audio file you want to close
         */
        void Close(AudioFile& file);

        f32 m_masterVolume = 1.0f;

        DynamicLibrary m_pluginLibrary;
        AudioPlugin* m_audioPlugin = nullptr;

        AudioChannel m_channels[MAX_AUDIO_CHANNELS];

        HashMap<EmitterHandle, AudioEmitter> m_emitters;
        HashMap<UUID, AudioFile> m_audioFiles;
    };
}  // namespace C3D
