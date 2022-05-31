
#include "resource_system.h"

#include "core/c3d_string.h"
#include "core/logger.h"
#include "core/memory.h"

// Default loaders
#include "resources/loaders/image_loader.h"

namespace C3D
{
	ResourceSystem::ResourceSystem(): m_initialized(false), m_config(), m_registeredLoaders(nullptr)
	{
	}

	bool ResourceSystem::Init(const ResourceSystemConfig& config)
	{
		Logger::PushPrefix("RESOURCE_SYSTEM");

		if (config.maxLoaderCount == 0)
		{
			Logger::Fatal("Init() failed because config.maxLoaderCount == 0");
			return false;
		}

		m_config = config;

		m_registeredLoaders = Memory::Allocate<ResourceLoader>(m_config.maxLoaderCount, MemoryType::ResourceLoader);
		const u32 count = m_config.maxLoaderCount;
		for (u32 i = 0; i < count; i++)
		{
			m_registeredLoaders[i].id = INVALID_ID;
		}

		// NOTE: Auto-register known loader types here
		if (const ImageLoader loader; !RegisterLoader(loader))
		{
			Logger::Fatal("RegisterLoader() failed for image loader");
			return false;
		}

		Logger::PrefixInfo("RESOURCE_SYSTEM", "Initialized with base path '{}'", m_config.assetBasePath);

		m_initialized = true;
		return true;
	}

	void ResourceSystem::Shutdown() const
	{
		Memory::Free(m_registeredLoaders, sizeof(ResourceLoader) * m_config.maxLoaderCount, MemoryType::ResourceLoader);
	}

	bool ResourceSystem::RegisterLoader(const ResourceLoader& newLoader) const
	{
		if (!m_initialized) return false;

		const u32 count = m_config.maxLoaderCount;
		// Ensure no loaders for the given type already exist
		for (u32 i = 0; i < count; i++)
		{
			if (const ResourceLoader* loader = &m_registeredLoaders[i]; loader->id != INVALID_ID)
			{
				if (loader->type == newLoader.type)
				{
					Logger::PrefixError("RESOURCE_SYSTEM", "RegisterLoader() - A loader of type '{}' already exists so the new one will not be registered", static_cast<u8>(newLoader.type));
					return false;
				}
				if (loader->customType && StringLength(loader->customType) > 0 && IEquals(loader->customType, newLoader.customType))
				{
					Logger::PrefixError("RESOURCE_SYSTEM", "RegisterLoader() - A loader of custom type '{}' already exists so the new one will not be registered", newLoader.customType);
					return false;
				}
			}
		}

		for (u32 i = 0; i < count; i++)
		{
			if (m_registeredLoaders[i].id == INVALID_ID)
			{
				m_registeredLoaders[i] = newLoader;
				m_registeredLoaders[i].id = i;

				Logger::PrefixTrace("RESOURCE_SYSTEM", "Loader registered");
				return true;
			}
		}

		Logger::PrefixError("RESOURCE_SYSTEM", "RegisterLoader() - Could not find a free slot for the new resource loader. Increase config.maxLoaderCount");
		return false;
	}

	bool ResourceSystem::Load(const string& name, const ResourceType type, Resource* outResource) const
	{
		if (m_initialized && type != ResourceType::Custom)
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (ResourceLoader* loader = &m_registeredLoaders[i]; loader->id != INVALID_ID && loader->type == type)
				{
					return Load(name, loader, outResource);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		Logger::PrefixError("RESOURCE_SYSTEM", "Load() No loader for type '{}' was found", static_cast<u8>(type));
		return false;
	}

	bool ResourceSystem::LoadCustom(const string& name, const string& customType, Resource* outResource) const
	{
		if (m_initialized && !customType.empty())
		{
			// Select a loader
			const u32 count = m_config.maxLoaderCount;
			for (u32 i = 0; i < count; i++)
			{
				if (ResourceLoader* loader = &m_registeredLoaders[i]; loader->id != INVALID_ID && IEquals(loader->customType, customType))
				{
					return Load(name, loader, outResource);
				}
			}
		}

		outResource->loaderId = INVALID_ID;
		Logger::PrefixError("RESOURCE_SYSTEM", "Load() No loader for type '{}' was found", customType);
		return false;
	}

	void ResourceSystem::Unload(Resource* resource) const
	{
		if (m_initialized && resource)
		{
			if (resource->loaderId != INVALID_ID)
			{
				if (ResourceLoader* loader = &m_registeredLoaders[resource->loaderId]; loader->id != INVALID_ID)
				{
					loader->Unload(resource);
				}
			}
		}
	}

	const char* ResourceSystem::GetBasePath() const
	{
		if (m_initialized) return m_config.assetBasePath;

		Logger::PrefixError("RESOURCE_SYSTEM", "GetBasePath() called before initialization. Returning empty string");
		return "";
	}

	bool ResourceSystem::Load(const string& name, ResourceLoader* loader, Resource* outResource)
	{
		if (name.empty() || !outResource) return false;
		if (!loader)
		{
			outResource->loaderId = INVALID_ID;
			return false;
		}

		outResource->loaderId = loader->id;
		return loader->Load(name, outResource);
	}
}
