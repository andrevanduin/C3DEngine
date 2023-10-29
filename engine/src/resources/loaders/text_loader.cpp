
#include "text_loader.h"

#include "core/engine.h"
#include "core/logger.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "TEXT_LOADER";

    ResourceLoader<TextResource>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, MemoryType::String, ResourceType::Text, nullptr, "")
    {}

    bool ResourceLoader<TextResource>::Load(const char* name, TextResource& resource) const
    {
        if (std::strlen(name) == 0) return false;

        // TODO: try different extensions
        auto fullPath = String::FromFormat("{}/{}/{}", Resources.GetBasePath(), typePath, name);

        File file;
        if (!file.Open(fullPath, FileModeRead))
        {
            ERROR_LOG("Unable to open file for text reading: '{}'.", fullPath);
            return false;
        }

        resource.fullPath = fullPath;

        u64 fileSize = 0;
        if (!file.Size(&fileSize))
        {
            ERROR_LOG("Unable to read size of file: '{}'.", fullPath);
            file.Close();
            return false;
        }

        // TODO: should be using an allocator here
        resource.text.Reserve(fileSize);
        resource.name = name;

        if (!file.ReadAll(resource.text))
        {
            ERROR_LOG("Unable to read text file: '{}'.", fullPath);
            file.Close();
            return false;
        }

        file.Close();
        return true;
    }

    void ResourceLoader<TextResource>::Unload(TextResource& resource)
    {
        resource.text.Destroy();
        resource.name.Destroy();
        resource.fullPath.Destroy();
    }
}  // namespace C3D
