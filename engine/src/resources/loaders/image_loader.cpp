
#include "image_loader.h"

#include "core/logger.h"
#include "math/c3d_math.h"
#include "platform/file_system.h"
#include "resources/resource_types.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "core/engine.h"
#include "stb/stb_image.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "IMAGE_LOADER";

    ResourceLoader<Image>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, MemoryType::Texture, ResourceType::Image, nullptr, "textures")
    {}

    bool ResourceLoader<Image>::Load(const char* name, Image& resource) const { return Load(name, resource, {}); }

    bool ResourceLoader<Image>::Load(const char* name, Image& resource, const ImageLoadParams& params) const
    {
        if (std::strlen(name) == 0) return false;

        constexpr i32 requiredChannelCount = 4;
        stbi_set_flip_vertically_on_load_thread(params.flipY);

        String fullPath(512);

        // Try different extensions
        const char* extensions[IMAGE_LOADER_EXTENSION_COUNT] = { "tga", "png", "jpg", "bmp" };
        bool found                                           = false;

        for (const auto extension : extensions)
        {
            const auto formatStr = "{}/{}/{}.{}";
            fullPath             = String::FromFormat(formatStr, Resources.GetBasePath(), typePath, name, extension);
            // Check if the requested file exists with the current extension
            if (File::Exists(fullPath))
            {
                // It exists so we break out of the loop
                found = true;
                break;
            }
        }

        if (!found)
        {
            ERROR_LOG("Failed to find file: '{}' with any supported extension.", name);
            return false;
        }

        // Take a copy of the resource path and name
        resource.fullPath = fullPath;
        resource.name     = name;

        File f;
        f.Open(fullPath, FileModeRead | FileModeBinary);

        u64 fileSize;
        if (!f.Size(&fileSize))
        {
            ERROR_LOG("Failed to get file size for '{}'.", fullPath);
            f.Close();
            return false;
        }

        i32 width;
        i32 height;
        i32 channelCount;

        // Allocate enough space for everything in the file
        const auto rawData = Memory.Allocate<u8>(MemoryType::Texture, fileSize);
        if (!rawData)
        {
            ERROR_LOG("Unable to allocate memory to store raw data for '{}'.", fullPath);
            f.Close();
            return false;
        }

        // Actually read our file and store the result in rawData
        u64 bytesRead     = 0;
        const auto result = f.ReadAll(reinterpret_cast<char*>(rawData), &bytesRead);
        f.Close();

        // Check if we could actually successfully read the data
        if (!result)
        {
            ERROR_LOG("Unable to read data for '{}'.", fullPath);
            return false;
        }

        u8* data = stbi_load_from_memory(rawData, static_cast<i32>(fileSize), &width, &height, &channelCount, requiredChannelCount);
        if (!data)
        {
            ERROR_LOG("STBI failed to load data from memory for '{}'.", fullPath);
            return false;
        }

        Memory.Free(rawData);

        resource.pixels       = data;
        resource.width        = width;
        resource.height       = height;
        resource.channelCount = requiredChannelCount;
        // To determine how many mip levels we will have, we first take the largest dimension then we take the base-2 log to see how many
        // times we can divide it by 2. Then we floor that value to ensure we are working on integer level and add 1 for the base level.
        resource.mipLevels = Floor(Log2(Max(width, height))) + 1;

        return true;
    }

    void ResourceLoader<Image>::Unload(Image& resource)
    {
        // Free the pixel data loaded in by STBI
        if (resource.pixels)
        {
            stbi_image_free(resource.pixels);
            resource.pixels = nullptr;
        }

        resource.fullPath.Destroy();
        resource.name.Destroy();
    }
}  // namespace C3D
