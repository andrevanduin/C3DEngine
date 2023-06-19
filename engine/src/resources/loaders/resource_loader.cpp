
#include "resource_loader.h"

#include "core/logger.h"

namespace C3D
{
    IResourceLoader::IResourceLoader(const SystemManager* pSystemsManager, const char* name,
                                     const MemoryType memoryType, const ResourceType type, const char* customType,
                                     const char* path)
        : id(INVALID_ID),
          type(type),
          customType(customType),
          typePath(path),
          m_logger(name),
          m_memoryType(memoryType),
          m_pSystemsManager(pSystemsManager)
    {}
}  // namespace C3D
