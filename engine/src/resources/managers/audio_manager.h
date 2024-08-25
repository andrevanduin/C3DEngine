
#pragma once
#include "core/audio/audio_file.h"
#include "core/audio/audio_types.h"
#include "core/defines.h"
#include "resource_manager.h"

namespace C3D
{
    struct AudioFileParams
    {
        AudioType type;
        u64 chunkSize;
    };

    template <>
    class C3D_API ResourceManager<AudioFile> final : public IResourceManager
    {
    public:
        ResourceManager();

        bool Init() override;

        bool Read(const String& name, AudioFile& resource, const AudioFileParams& params) const;
        void Cleanup(AudioFile& resource) const;

    private:
        mutable mp3dec_t m_mp3Decoder;
    };
}  // namespace C3D