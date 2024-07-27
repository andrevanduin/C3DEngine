
#include "resource_loader.h"

#include "core/logger.h"

namespace C3D
{
    IResourceLoader::IResourceLoader(const MemoryType memoryType, const ResourceType type, const char* customType, const char* path)
        : type(type), customType(customType), typePath(path), m_memoryType(memoryType)
    {}
}  // namespace C3D
