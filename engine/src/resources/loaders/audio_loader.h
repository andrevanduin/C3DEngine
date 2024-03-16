
#pragma once
#include "core/audio/audio_file.h"
#include "core/audio/audio_types.h"
#include "core/defines.h"
#include "resource_loader.h"

namespace C3D
{
    struct AudioFileParams
    {
        AudioType type;
        u64 chunkSize;
    };

    template <>
    class C3D_API ResourceLoader<AudioFile> final : public IResourceLoader
    {
    public:
        explicit ResourceLoader(const SystemManager* pSystemsManager);

        bool Init() override;

        bool Load(const char* name, AudioFile& resource, const AudioFileParams& params) const;
        static void Unload(AudioFile& resource);

    private:
        mutable mp3dec_t m_mp3Decoder;
    };
}  // namespace C3D