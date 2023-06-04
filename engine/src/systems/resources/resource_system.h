
#pragma once
#include "systems/system.h"
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

	class ResourceSystem final : public SystemWithConfig<ResourceSystemConfig>
	{
	public:
		explicit ResourceSystem(const Engine* engine);

		bool Init(const ResourceSystemConfig& config) override;

		void Shutdown() override;

		C3D_API bool RegisterLoader(IResourceLoader* newLoader);

		template <typename Type>
		C3D_API bool Load(const char* name, Type& resource);

		template <typename Type, typename Params>
		C3D_API bool Load(const char* name, Type& resource, const Params& params);

		template <typename Type>
		C3D_API void Unload(Type& resource) const;

		C3D_API const char* GetBasePath() const;

	private:
		template <typename Type>
		static bool LoadInternal(const char* name, ResourceLoader<Type>* loader, Type& resource);

		template <typename Type, typename Params>
		static bool LoadInternalParams(const char* name, ResourceLoader<Type>* loader, Type& resource, const Params& params);

		DynamicArray<IResourceLoader*> m_registeredLoaders;

		const char* m_loaderTypes[ToUnderlying(ResourceType::MaxValue)];
	};

	template <typename Type>
	bool ResourceSystem::Load(const char* name, Type& resource)
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
					return LoadInternal(name, resLoader, resource);
				}
			}
		}

		resource.loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type of resource at '{}' was found", name);
		return false;
	}

	template <typename Type, typename Params>
	bool ResourceSystem::Load(const char* name, Type& resource, const Params& params)
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
					return LoadInternalParams(name, resLoader, resource, params);
				}
			}
		}

		resource.loaderId = INVALID_ID;
		m_logger.Error("Load() No loader for type of resource at '{}' was found", name);
		return false;
	}

	template <typename Type>
	void ResourceSystem::Unload(Type& resource) const
	{
		if (m_initialized)
		{
			if (resource.loaderId != INVALID_ID)
			{
				if (auto loader = m_registeredLoaders[resource.loaderId]; loader->id != INVALID_ID)
				{
					auto pLoader = dynamic_cast<ResourceLoader<Type>*>(loader);
					pLoader->Unload(resource);
				}
			}
		}
	}

	template <typename Type>
	bool ResourceSystem::LoadInternal(const char* name, ResourceLoader<Type>* loader, Type& resource)
	{
		if (std::strlen(name) == 0) return false;
		if (!loader)
		{
			resource.loaderId = INVALID_ID;
			return false;
		}

		resource.loaderId = loader->id;
		return loader->Load(name, resource);
	}

	template <typename Type, typename Params>
	bool ResourceSystem::LoadInternalParams(const char* name, ResourceLoader<Type>* loader, Type& resource, const Params& params)
	{
		if (std::strlen(name) == 0) return false;
		if (!loader)
		{
			resource.loaderId = INVALID_ID;
			return false;
		}

		resource.loaderId = loader->id;
		return loader->Load(name, resource, params);
	}
}
