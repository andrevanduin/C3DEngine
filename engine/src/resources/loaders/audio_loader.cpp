
#include "audio_loader.h"

#include <stb/stb_vorbis.h>

#include "core/string_utils.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr auto AUDIO_EXTENSION_COUNT = 2;

    ResourceLoader<AudioFile>::ResourceLoader() : IResourceLoader(MemoryType::AudioType, ResourceType::AudioFile, nullptr, "audio") {}

    bool ResourceLoader<AudioFile>::Init()
    {
        mp3dec_init(&m_mp3Decoder);
        return true;
    }

    bool ResourceLoader<AudioFile>::Load(const char* name, AudioFile& resource, const AudioFileParams& params) const
    {
        if (std::strlen(name) == 0) return false;

        String fullPath(512);

        // Try different extensions
        String extensions[AUDIO_EXTENSION_COUNT] = { "ogg", "mp3" };
        String pickedExtension;

        for (auto extension : extensions)
        {
            const auto formatStr = "{}/{}/{}.{}";
            fullPath             = String::FromFormat(formatStr, Resources.GetBasePath(), typePath, name, extension);
            // Check if the requested file exists with the current extension
            if (File::Exists(fullPath))
            {
                // It exists so we break out of the loop
                pickedExtension = extension;
                break;
            }
        }

        if (!pickedExtension)
        {
            ERROR_LOG("Failed to find file: '{}' with any supported extension.", name);
            return false;
        }

        // Take a copy of the resource path and name
        resource.fullPath = fullPath;
        resource.name     = name;

        if (pickedExtension == "ogg")
        {
            resource.LoadVorbis(params.type, params.chunkSize, fullPath);
        }
        else
        {
            resource.LoadMp3(params.type, params.chunkSize, fullPath, &m_mp3Decoder);
        }

        return true;
    }

    void ResourceLoader<AudioFile>::Unload(AudioFile& resource)
    {
        resource.fullPath.Destroy();
        resource.name.Destroy();
        resource.Unload();
    }
}  // namespace C3D