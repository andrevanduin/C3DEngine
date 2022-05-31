
#include "material_system.h"

#include "core/logger.h"
#include "core/c3d_string.h"
#include "core/memory.h"

#include "services/services.h"

// TEMP: resource system
#include "platform/filesystem.h"
// TEMP end

namespace C3D
{
	MaterialSystem::MaterialSystem()
		: m_initialized(false), m_config(), m_defaultMaterial(), m_registeredMaterials(nullptr)
	{
	}

	bool MaterialSystem::Init(const MaterialSystemConfig& config)
	{
		if (config.maxMaterialCount == 0)
		{
			Logger::PrefixError("TEXTURE_SYSTEM", "config.maxTextureCount must be > 0");
			return false;
		}

		m_config = config;

		// Allocate enough memory for the max number of textures that we will be using
		m_registeredMaterials = Memory::Allocate<Material>(config.maxMaterialCount, MemoryType::MaterialInstance);
		// Set the id of all the textures to invalid
		for (u32 i = 0; i < m_config.maxMaterialCount; i++)
		{
			m_registeredMaterials[i].id = INVALID_ID;
			m_registeredMaterials[i].generation = INVALID_ID;
			m_registeredMaterials[i].internalId = INVALID_ID;
		}

		// Ensure that we have enough space for all our textures
		m_registeredMaterialTable.reserve(config.maxMaterialCount);

		if (!CreateDefaultMaterial())
		{
			Logger::PrefixError("MATERIAL_SYSTEM", "Failed to create default material");
			return false;
		}

		m_initialized = true;
		return true;
	}

	void MaterialSystem::Shutdown()
	{
		Logger::PrefixInfo("TEXTURE_SYSTEM", "Destroying all loaded materials");
		for (u32 i = 0; i < m_config.maxMaterialCount; i++)
		{
			if (m_registeredMaterials[i].id != INVALID_ID)
			{
				DestroyMaterial(&m_registeredMaterials[i]);
			}
		}

		Logger::PrefixInfo("TEXTURE_SYSTEM", "Destroying default material");
		DestroyMaterial(&m_defaultMaterial);
	}

	Material* MaterialSystem::Acquire(const string& name)
	{
		MaterialConfig config;
		// TODO: Try different extensions
		const auto fullPath = "assets/materials/" + name + ".cmt";

		if (!LoadConfigurationFile(fullPath, &config))
		{
			Logger::PrefixError("MATERIAL_SYSTEM", "Failed to load material file: {}. Nullptr will be returned", fullPath);
			return nullptr;
		}
		// Now we can acquire from the loaded config
		return AcquireFromConfig(config);
	}

	Material* MaterialSystem::AcquireFromConfig(const MaterialConfig& config)
	{
		if (IEquals(config.name, DEFAULT_MATERIAL_NAME))
		{
			return &m_defaultMaterial;
		}

		const auto& it = m_registeredMaterialTable.find(config.name);
		// No Material could be found for this name
		if (it == m_registeredMaterialTable.end())
		{
			m_registeredMaterialTable.emplace(config.name, MaterialReference{ 0, INVALID_ID, config.autoRelease });
		}

		auto& ref = m_registeredMaterialTable[config.name];
		ref.referenceCount++;

		if (ref.handle == INVALID_ID)
		{
			// No material exists yet. Let's find a free index for it
			const u32 count = m_config.maxMaterialCount;
			Material* mat = nullptr;
			for (u32 i = 0; i < count; i++)
			{
				if (m_registeredMaterials[i].id == INVALID_ID)
				{
					ref.handle = i;
					mat = &m_registeredMaterials[i];
					break;
				}
			}

			if (!mat || ref.handle == INVALID_ID)
			{
				Logger::PrefixFatal("MATERIAL_SYSTEM", "No more free space for materials. Adjust the configuration to allow more");
				return nullptr;
			}

			// Create a new Material
			if (!LoadMaterial(config, mat))
			{
				Logger::PrefixError("MATERIAL_SYSTEM", "Failed to load material {}", config.name);
				return nullptr;
			}

			if (mat->generation == INVALID_ID) mat->generation = 0;
			else mat->generation++;

			mat->id = ref.handle;
			Logger::PrefixTrace("MATERIAL_SYSTEM", "Material {} did not exist yet. Created and the refCount is now {}", config.name, ref.referenceCount);
		}
		else
		{
			Logger::PrefixTrace("MATERIAL_SYSTEM", "Material {} already exists. The refCount is now {}", config.name, ref.referenceCount);
		}

		return &m_registeredMaterials[ref.handle];
	}

	void MaterialSystem::Release(const string& name)
	{
		if (IEquals(name, DEFAULT_MATERIAL_NAME))
		{
			Logger::PrefixWarn("MATERIAL_SYSTEM", "Tried to release {}. This happens automatically on shutdown", DEFAULT_MATERIAL_NAME);
			return;
		}

		const auto it = m_registeredMaterialTable.find(name);
		if (it == m_registeredMaterialTable.end())
		{
			Logger::PrefixWarn("MATERIAL_SYSTEM", "Tried to release a material that does not exist: {}", name);
			return;
		}

		auto& ref = it->second;
		ref.referenceCount--;

		if (ref.referenceCount == 0 && ref.autoRelease)
		{
			// This material is marked for auto release and we are holding no more references to it
			Material* mat = &m_registeredMaterials[ref.handle];

			// Destroy the material
			DestroyMaterial(mat);

			// Remove the reference
			m_registeredMaterialTable.erase(it);

			Logger::PrefixInfo("MATERIAL_SYSTEM", "Released material {}. The texture was unloaded because refCount = 0 and autoRelease = true", name);
			return;
		}

		Logger::PrefixInfo("MATERIAL_SYSTEM", "Released material {}. The material now has a refCount = {} (autoRelease = {})", name, ref.referenceCount, ref.autoRelease);
	}

	Material* MaterialSystem::GetDefault()
	{
		if (!m_initialized)
		{
			Logger::PrefixFatal("MATERIAL_SYSTEM", "Tried to get the default material before system is initialized");
			return nullptr;
		}
		return &m_defaultMaterial;
	}

	bool MaterialSystem::CreateDefaultMaterial()
	{
		Memory::Zero(&m_defaultMaterial, sizeof(Material));

		m_defaultMaterial.id = INVALID_ID;
		m_defaultMaterial.generation = INVALID_ID;
		StringNCopy(m_defaultMaterial.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
		m_defaultMaterial.diffuseColor = vec4(1);
		m_defaultMaterial.diffuseMap.use = TextureUse::Diffuse;
		m_defaultMaterial.diffuseMap.texture = Textures.GetDefaultTexture();

		if (!Renderer.CreateMaterial(&m_defaultMaterial))
		{
			Logger::PrefixError("MATERIAL_SYSTEM", "Failed to acquire renderer resources for the the default material");
			return false;
		}

		return true;
	}

	bool MaterialSystem::LoadMaterial(const MaterialConfig& config, Material* mat)
	{
		Memory::Zero(mat, sizeof(Material));

		// Name
		StringNCopy(mat->name, config.name, MATERIAL_NAME_MAX_LENGTH);
		// Diffuse color
		mat->diffuseColor = config.diffuseColor;

		// Diffuse map
		if (StringLength(config.diffuseMapName) > 0)
		{
			mat->diffuseMap.use = TextureUse::Diffuse;
			mat->diffuseMap.texture = Textures.Acquire(config.diffuseMapName, true);

			if (!mat->diffuseMap.texture)
			{
				Logger::PrefixWarn("MATERIAL_SYSTEM", "Unable to load texture '{}' for material '{}', using the default", config.diffuseMapName, mat->name);
				mat->diffuseMap.texture = Textures.GetDefaultTexture();
			}
		}
		else
		{
			// NOTE: this does nothing since we already zeroed the memory. It's only for clarity
			mat->diffuseMap.use = TextureUse::Unknown;
			mat->diffuseMap.texture = nullptr;
		}

		// TODO: other maps

		if (!Renderer.CreateMaterial(mat))
		{
			Logger::PrefixError("MATERIAL_SYSTEM", "Failed to acquire renderer resources for material: {}", mat->name);
			return false;
		}

		return true;
	}

	void MaterialSystem::DestroyMaterial(Material* mat)
	{
		Logger::PrefixTrace("MATERIAL_SYSTEM", "Destroying material '{}'", mat->name);

		// If the diffuseMap has a texture we release it
		if (mat->diffuseMap.texture)
		{
			Textures.Release(mat->diffuseMap.texture->name);
		}

		// Release renderer resources
		Renderer.DestroyMaterial(mat);

		// Zero out memory and invalidate ids
		Memory::Zero(mat, sizeof(Material));
		mat->id = INVALID_ID;
		mat->generation = INVALID_ID;
		mat->internalId = INVALID_ID;
	}

	bool MaterialSystem::LoadConfigurationFile(const string& path, MaterialConfig* outConfig)
	{
		File file;
		if (!file.Open(path, FileModeRead))
		{
			Logger::PrefixError("MATERIAL_SYSTEM", "LoadConfigurationFile() - unable to open material file for reading: '{}'", path);
			return false;
		}

		string line;
		// Prepare for strings of up to 512 characters so we don't needlessly resize
		line.reserve(512); 

		u32 lineNumber = 0;
		while (file.ReadLine(line))
		{
			Trim(line);

			// Skip blank lines and comments
			if (line.empty() || line[0] == '#')
			{
				lineNumber++;
				continue;
			}

			// Find the '=' symbol
			const auto equalIndex = line.find('=');
			if (equalIndex == string::npos)
			{
				Logger::PrefixWarn("MATERIAL_SYSTEM", "Potential formatting issue found in file '': '=' token not found. Skipping line {}.", path, lineNumber);
				lineNumber++;
				continue;
			}

			// Get the variable name (which is all the characters up to the '=' and trim
			auto varName = line.substr(0, equalIndex);
			Trim(varName);

			// Get the value (which is all the characters after the '=' and trim
			auto value = line.substr(equalIndex + 1);
			Trim(value);

			if (IEquals(varName, "version"))
			{
				// TODO: version
			}
			else if (IEquals(varName, "name"))
			{
				StringNCopy(outConfig->name, value.data(), MATERIAL_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "diffuseMapName"))
			{
				StringNCopy(outConfig->diffuseMapName, value.data(), TEXTURE_NAME_MAX_LENGTH);
			}
			else if (IEquals(varName, "diffuseColor"))
			{
				if (!StringToVec4(value.data(), &outConfig->diffuseColor))
				{
					Logger::PrefixWarn("MATERIAL_SYSTEM", "Error parsing diffuseColor in file ''. Using default of white instead", path);
					outConfig->diffuseColor = vec4(1);
				}
			}

			// TODO: more fields

			lineNumber++;
		}

		file.Close();
		return true;
	}
}
