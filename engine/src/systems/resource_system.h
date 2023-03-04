
#pragma once
#include "containers/dynamic_array.h"
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

		void Shutdown();

		C3D_API bool RegisterLoader(IResourceLoader* newLoader);

		template <typename Type>
		C3D_API bool Load(const char* name, Type* outResource);

		template <typename Type, typename Params>
		C3D_API bool Load(const char* name, Type* outResource, const Params& params);

		template <typename Type>
		C3D_API void Unload(Type * resource) const;

		C3D_API const char* GetBasePath() const;

	private:
		template <typename Type>
		bool LoadInternal(const char* name, ResourceLoader<Type>* loader, Type* outResource);

		template <typename Type, typename Params>
		bool LoadInternalParams(const char* name, ResourceLoader<Type>* loader, Type* outResource, const Params& params);

		LoggerInstance m_logger;

		bool m_initialized;

		ResourceSystemConfig m_config;

		DynamicArray<IResourceLoader*> m_registeredLoaders;

		const char* m_loaderTypes[ToUnderlying(ResourceType::MaxValue)];
	};

	template <typename Type>
	bool ResourceSystem::Load(const char* name, Type* outResource)
	{
		if (m_initialized)
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (auto loader = m_registeredLoaders[i]; loader->id != INVALID_ID && typeid(*loader) == typeid(ResourceLoader<Type>))
				{
					auto resLoader = dynamic_cast<ResourceLoader<Type>*>(loader);
					return LoadInternal(name, resLoader, outResource);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type of resource at '{}' was found", name);
		return false;
	}

	template <typename Type, typename Params>
	bool ResourceSystem::Load(const char* name, Type* outResource, const Params& params)
	{
		if (m_initialized)
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (auto loader = m_registeredLoaders[i]; loader->id != INVALID_ID && typeid(*loader) == typeid(ResourceLoader<Type>))
				{
					auto resLoader = dynamic_cast<ResourceLoader<Type>*>(loader);
					return LoadInternalParams(name, resLoader, outResource, params);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type of resource at '{}' was found", name);
		return false;
	}

	template <typename Type>
	void ResourceSystem::Unload(Type* resource) const
	{
		if (m_initialized && resource)
		{
			if (resource->loaderId != INVALID_ID)
			{
				if (auto loader = m_registeredLoaders[resource->loaderId]; loader->id != INVALID_ID)
				{
					auto pLoader = dynamic_cast<ResourceLoader<Type>*>(loader);
					pLoader->Unload(resource);
				}
			}
		}
	}

	template <typename Type>
	bool ResourceSystem::LoadInternal(const char* name, ResourceLoader<Type>* loader, Type* outResource)
	{
		if (std::strlen(name) == 0 || !outResource) return false;
		if (!loader)
		{
			outResource->loaderId = INVALID_ID;
			return false;
		}

		outResource->loaderId = loader->id;
		return loader->Load(name, outResource);
	}

	template <typename Type, typename Params>
	bool ResourceSystem::LoadInternalParams(const char* name, ResourceLoader<Type>* loader, Type* outResource, const Params& params)
	{
		if (std::strlen(name) == 0 || !outResource) return false;
		if (!loader)
		{
			outResource->loaderId = INVALID_ID;
			return false;
		}

		outResource->loaderId = loader->id;
		return loader->Load(name, outResource, params);
	}
}
