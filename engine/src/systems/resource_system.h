
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

		template<typename T>
		C3D_API bool Load(const char* name, ResourceType type, T* outResource);

		template<typename T>
		C3D_API bool LoadCustom(const char* name, const char* customType, T* outResource);

		template<typename T>
		C3D_API void Unload(T* resource) const;

		C3D_API const char* GetBasePath() const;

	private:
		template<typename T>
		static bool Load(const char* name, ResourceLoader<T>* loader, T* outResource);

		LoggerInstance m_logger;

		bool m_initialized;

		ResourceSystemConfig m_config;

		DynamicArray<IResourceLoader*> m_registeredLoaders;

		const char* m_loaderTypes[ToUnderlying(ResourceType::MaxValue)];
	};

	template <typename T>
	bool ResourceSystem::Load(const char* name, const ResourceType type, T* outResource)
	{
		if (m_initialized && type != ResourceType::Custom)
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (auto loader = m_registeredLoaders[i]; loader->id != INVALID_ID && loader->type == type)
				{
					return Load(name, dynamic_cast<ResourceLoader<T>*>(loader), outResource);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type '{}' was found", m_loaderTypes[ToUnderlying(type)]);
		return false;
	}

	template <typename T>
	bool ResourceSystem::LoadCustom(const char* name, const char* customType, T* outResource)
	{
		if (m_initialized && StringLength(customType) > 0)
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (auto loader = m_registeredLoaders[i]; loader->id != INVALID_ID && IEquals(loader->customType, customType))
				{
					return Load(name, static_cast<ResourceLoader<T>>(loader), outResource);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type '{}' was found", customType);
		return false;
	}

	template <typename T>
	void ResourceSystem::Unload(T* resource) const
	{
		if (m_initialized && resource)
		{
			if (resource->loaderId != INVALID_ID)
			{
				if (auto loader = m_registeredLoaders[resource->loaderId]; loader->id != INVALID_ID)
				{
					const auto pLoader = dynamic_cast<ResourceLoader<T>*>(loader);
					pLoader->Unload(resource);
				}
			}
		}
	}

	template <typename T>
	bool ResourceSystem::Load(const char* name, ResourceLoader<T>* loader, T* outResource)
	{
		if (StringLength(name) == 0 || !outResource) return false;
		if (!loader)
		{
			outResource->loaderId = INVALID_ID;
			return false;
		}

		outResource->loaderId = loader->id;
		return loader->Load(name, outResource);
	}
}
