
#pragma once
#include "core/logger.h"
#include "core/memory.h"
#include "resources/resource_types.h"

namespace C3D
{
	class IResourceLoader
	{
	public:
		IResourceLoader(const char* name, MemoryType memoryType, ResourceType type, const char* customType, const char* path);

		IResourceLoader(const IResourceLoader& other) = delete;
		IResourceLoader(IResourceLoader&& other) = delete;

		IResourceLoader& operator=(const IResourceLoader& other) = default;
		IResourceLoader& operator=(IResourceLoader&& other) = delete;

		virtual ~IResourceLoader() = default;

		u32 id;
		ResourceType type;

		const char* customType;
		const char* typePath;
	protected:
		LoggerInstance m_logger;
		MemoryType m_memoryType;
	};

	template<typename T>
	class ResourceLoader final : public IResourceLoader
	{
	public:
		ResourceLoader() : IResourceLoader("NONE", MemoryType::Unknown, ResourceType::None, nullptr, nullptr) {}

		bool Load(const char* name, T* outResource) const;
	};
}
