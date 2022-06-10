
#pragma once
#include "core/logger.h"
#include "core/memory.h"
#include "resources/resource_types.h"

namespace C3D
{
	class ResourceLoader
	{
	public:
		ResourceLoader(const char* name, MemoryType memoryType, ResourceType type, const char* customType, const char* path);

		ResourceLoader(const ResourceLoader& other) = delete;
		ResourceLoader(ResourceLoader&& other) = delete;

		ResourceLoader& operator=(const ResourceLoader& other) = default;
		ResourceLoader& operator=(ResourceLoader&& other) = delete;

		virtual ~ResourceLoader() = default;

		virtual bool Load(const char* name, Resource* outResource) = 0;

		virtual void Unload(Resource* resource);

		u32 id;
		ResourceType type;

		const char* customType;
		const char* typePath;
	protected:
		LoggerInstance m_logger;
		MemoryType m_memoryType;
	};
}