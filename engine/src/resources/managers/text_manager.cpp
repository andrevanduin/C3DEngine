
#include "text_manager.h"

#include "core/engine.h"
#include "core/logger.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    ResourceManager<TextResource>::ResourceManager() : IResourceManager(MemoryType::String, ResourceType::Text, nullptr, "") {}

    bool ResourceManager<TextResource>::Read(const String& name, TextResource& resource) const
    {
        if (name.Empty())
        {
            ERROR_LOG("No valid name was provided.");
            return false;
        }

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

    void ResourceManager<TextResource>::Cleanup(TextResource& resource) const
    {
        resource.text.Destroy();
        resource.name.Destroy();
        resource.fullPath.Destroy();
    }
}  // namespace C3D
