
#pragma once
#include "core/defines.h"
#include "resources/resource_types.h"

namespace C3D
{
	struct ResourceSystemConfig
	{
		u32 maxLoaderCount;
		const char* assetBasePath; // Relative base path
	};

	class ResourceLoader
	{
	public:
		ResourceLoader(const ResourceType type, const char* customType, const char* path) : id(INVALID_ID), type(type), customType(customType), typePath(path) {}

		ResourceLoader(const ResourceLoader& other) = delete;
		ResourceLoader(ResourceLoader&& other) = delete;

		ResourceLoader& operator=(const ResourceLoader& other) = default;
		ResourceLoader& operator=(ResourceLoader&& other) = delete;

		virtual ~ResourceLoader() = default;

		virtual bool Load(const string& name, Resource* outResource) = 0;

		virtual void Unload(Resource* resource) = 0;

		u32 id;
		ResourceType type;

		const char* customType;
		const char* typePath;
	};

	class ResourceSystem
	{
	public:
		ResourceSystem();

		bool Init(const ResourceSystemConfig& config);

		void Shutdown() const;

		C3D_API bool RegisterLoader(const ResourceLoader& newLoader) const;

		C3D_API bool Load(const string& name, ResourceType type, Resource* outResource) const;

		C3D_API bool LoadCustom(const string& name, const string& customType, Resource* outResource) const;

		C3D_API void Unload(Resource* resource) const;

		C3D_API const char* GetBasePath() const;

	private:
		static bool Load(const string& name, ResourceLoader* loader, Resource* outResource);

		bool m_initialized;

		ResourceSystemConfig m_config;
		ResourceLoader* m_registeredLoaders;
	};
}