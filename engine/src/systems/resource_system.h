
#pragma once
#include "core/defines.h"
#include "core/logger.h"

#include "resources/resource_types.h"
#include "resources/loaders/resource_loader.h"

namespace C3D
{
	struct ResourceSystemConfig
	{
		u32 maxLoaderCount;
		const char* assetBasePath; // Relative base path
	};

	class ResourceSystem
	{
	public:
		ResourceSystem();

		bool Init(const ResourceSystemConfig& config);

		void Shutdown() const;

		C3D_API bool RegisterLoader(ResourceLoader* newLoader) const;

		C3D_API bool Load(const string& name, ResourceType type, Resource* outResource) const;

		C3D_API bool LoadCustom(const string& name, const string& customType, Resource* outResource) const;

		C3D_API void Unload(Resource* resource) const;

		C3D_API const char* GetBasePath() const;

	private:
		static bool Load(const string& name, ResourceLoader* loader, Resource* outResource);

		LoggerInstance m_logger;

		bool m_initialized;

		ResourceSystemConfig m_config;
		ResourceLoader* m_registeredLoaders;
	};
}
