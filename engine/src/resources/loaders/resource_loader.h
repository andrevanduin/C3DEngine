
#pragma once
#include "core/logger.h"
#include "resources/resource_types.h"

namespace C3D
{
	class Engine;

	class IResourceLoader
	{
	public:
		IResourceLoader(const Engine* engine, const char* name, MemoryType memoryType, ResourceType type, const char* customType, const char* path);

		IResourceLoader(const IResourceLoader& other) = delete;
		IResourceLoader(IResourceLoader&& other) = delete;

		IResourceLoader& operator=(const IResourceLoader& other) = default;
		IResourceLoader& operator=(IResourceLoader&& other) = delete;

		virtual ~IResourceLoader() = default;

		u32 id;
		ResourceType type;

		String customType;
		String typePath;
	protected:
		LoggerInstance<64> m_logger;
		MemoryType m_memoryType;

		const Engine* m_engine;
	};

	template<typename T>
	class ResourceLoader final : public IResourceLoader
	{
	public:
		explicit ResourceLoader(const Engine* engine) : IResourceLoader(engine, "NONE", MemoryType::Unknown, ResourceType::None, nullptr, nullptr) {}

		bool Load(const char* name, T& resource) const;
	};
}
