
#include "binary_loader.h"

#include "core/engine.h"
#include "core/logger.h"
#include "platform/file_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "BINARY_LOADER";

    ResourceLoader<BinaryResource>::ResourceLoader(const SystemManager* pSystemsManager)
        : IResourceLoader(pSystemsManager, MemoryType::Array, ResourceType::Binary, nullptr, "shaders")
    {}

    bool ResourceLoader<BinaryResource>::Load(const char* name, BinaryResource& resource) const
    {
        if (std::strlen(name) == 0) return false;

        // TODO: try different extensions
        auto fullPath = String::FromFormat("{}/{}/{}", Resources.GetBasePath(), typePath, name);

        File file;
        if (!file.Open(fullPath, FileModeRead | FileModeBinary))
        {
            ERROR_LOG("Unable to open file for binary reading: '{}'.", fullPath);
            return false;
        }

        resource.fullPath = fullPath;

        u64 fileSize = 0;
        if (!file.Size(&fileSize))
        {
            ERROR_LOG("Unable to binary read size of file: '{}'.", fullPath);
            file.Close();
            return false;
        }

        // TODO: should be using an allocator here
        resource.data = Memory.Allocate<char>(MemoryType::Array, fileSize);
        resource.name = name;

        if (!file.ReadAll(resource.data, &resource.size))
        {
            ERROR_LOG("Unable to read binary file: '{}'.", fullPath);
            file.Close();
            return false;
        }

        file.Close();
        return true;
    }

    void ResourceLoader<BinaryResource>::Unload(BinaryResource& resource)
    {
        Memory.Free(MemoryType::Array, resource.data);
        resource.data = nullptr;

        resource.name.Destroy();
        resource.fullPath.Destroy();
    }
}  // namespace C3D
