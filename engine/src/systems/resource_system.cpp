
#include "resource_system.h"

#include "core/c3d_string.h"
#include "core/logger.h"

// Default loaders
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/mesh_loader.h"
#include "resources/loaders/shader_loader.h"
#include "resources/loaders/text_loader.h"

#include "services/services.h"

namespace C3D
{
	static const char* LOADER_TYPES[ToUnderlying(ResourceType::MaxValue)];

	ResourceSystem::ResourceSystem()
		: m_logger("RESOURCE_SYSTEM"), m_initialized(false), m_config(), m_registeredLoaders(nullptr)
	{
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "None";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "Text";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "Binary";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "Image";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "Material";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "StaticMesh";
		LOADER_TYPES[ToUnderlying(ResourceType::Shader)] = "Shader";
		LOADER_TYPES[ToUnderlying(ResourceType::None)] = "Custom";
	}

	bool ResourceSystem::Init(const ResourceSystemConfig& config)
	{
		if (config.maxLoaderCount == 0)
		{
			m_logger.Fatal("Init() failed because config.maxLoaderCount == 0");
			return false;
		}

		m_config = config;

		m_registeredLoaders = Memory.Allocate<ResourceLoader>(m_config.maxLoaderCount, MemoryType::ResourceLoader);
		const u32 count = m_config.maxLoaderCount;
		for (u32 i = 0; i < count; i++)
		{
			m_registeredLoaders[i].id = INVALID_ID;
		}

		m_initialized = true;

		// NOTE: Auto-register known loader types here
		// NOTE: Different way of allocating this???
		ResourceLoader* loaders[] =
		{
			new TextLoader(), new BinaryLoader(), new ImageLoader(), new MaterialLoader(), new ShaderLoader(), new MeshLoader()
		};

		for (const auto loader : loaders)
		{
			if (!RegisterLoader(loader))
			{
				m_logger.Fatal("RegisterLoader() failed for {} loader", LOADER_TYPES[ToUnderlying(loader->type)]);
				return false;
			}

			// RegisterLoader copies the loader so we can safely delete
			// our pointers to cleanup after ourselves
			delete loader; 
		}

		m_logger.Info("Initialized with base path '{}'", m_config.assetBasePath);
		return true;
	}

	void ResourceSystem::Shutdown() const
	{
		Memory.Free(m_registeredLoaders, sizeof(ResourceLoader) * m_config.maxLoaderCount, MemoryType::ResourceLoader);
	}

	bool ResourceSystem::RegisterLoader(ResourceLoader* newLoader) const
	{
		if (!m_initialized) return false;

		const u32 count = m_config.maxLoaderCount;
		// Ensure no loaders for the given type already exist
		for (u32 i = 0; i < count; i++)
		{
			if (const ResourceLoader* loader = &m_registeredLoaders[i]; loader->id != INVALID_ID)
			{
				if (loader->type == newLoader->type)
				{
					m_logger.Error("RegisterLoader() - A loader of type '{}' already exists so the new one will not be registered", LOADER_TYPES[ToUnderlying(newLoader->type)]);
					return false;
				}
				if (loader->customType && StringLength(loader->customType) > 0 && IEquals(loader->customType, newLoader->customType))
				{
					m_logger.Error("RegisterLoader() - A loader of custom type '{}' already exists so the new one will not be registered", newLoader->customType);
					return false;
				}
			}
		}

		for (u32 i = 0; i < count; i++)
		{
			if (m_registeredLoaders[i].id == INVALID_ID)
			{
				Memory.Copy(&m_registeredLoaders[i], newLoader, sizeof(ResourceLoader));
				m_registeredLoaders[i].id = i;

				m_logger.Trace("Loader registered");
				return true;
			}
		}

		m_logger.Error("RegisterLoader() - Could not find a free slot for the new resource loader. Increase config.maxLoaderCount");
		return false;
	}

	bool ResourceSystem::Load(const char* name, const ResourceType type, Resource* outResource) const
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
		m_logger.Error("Load() No loader for type '{}' was found", LOADER_TYPES[ToUnderlying(type)]);
		return false;
	}

	bool ResourceSystem::LoadCustom(const char* name, const char* customType, Resource* outResource) const
	{
		if (m_initialized && StringLength(customType) > 0)
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
		m_logger.Error("Load() No loader for type '{}' was found", customType);
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

		m_logger.Error("GetBasePath() called before initialization. Returning empty string");
		return "";
	}

	bool ResourceSystem::Load(const char* name, ResourceLoader* loader, Resource* outResource)
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
