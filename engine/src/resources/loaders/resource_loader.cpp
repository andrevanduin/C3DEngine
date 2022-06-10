
#include "resource_loader.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "services/services.h"

namespace C3D
{
	ResourceLoader::ResourceLoader(const char* name, const MemoryType memoryType, const ResourceType type, const char* customType, const char* path)
		: id(INVALID_ID), type(type), customType(customType), typePath(path), m_logger(name), m_memoryType(memoryType)
	{}

	void ResourceLoader::Unload(Resource* resource)
	{
		if (!resource)
		{
			m_logger.Warn("Unload() called with nullptr for resource");
			return;
		}

		u64 count = StringLength(resource->fullPath);
		if (count != 0)
		{
			Memory.Free(resource->fullPath, count + 1, MemoryType::String);
		}

		if (resource->name)
		{
			count = StringLength(resource->name);
			if (count != 0)
			{
				Memory.Free(resource->name, count + 1, MemoryType::String);
			}
		}

		if (resource->data)
		{
			Memory.Free(resource->data, resource->dataSize, m_memoryType);
			resource->data = nullptr;
			resource->dataSize = 0;
			resource->loaderId = INVALID_ID;
		}
	}
}
