
#include "material_system.h"

#include "core/logger.h"
#include "core/c3d_string.h"
#include "core/memory.h"

#include "renderer/renderer_types.h"
#include "renderer/renderer_frontend.h"
#include "resources/shader.h"

#include "services/services.h"
#include "systems/shader_system.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"

namespace C3D
{
	MaterialSystem::MaterialSystem()
		: System("MATERIAL_SYSTEM"), m_initialized(false), m_defaultMaterial(), m_registeredMaterials(nullptr),
		  m_materialLocations(), m_materialShaderId(0), m_uiLocations(), m_uiShaderId(0)
	{}

	bool MaterialSystem::Init(const MaterialSystemConfig& config)
	{
		if (config.maxMaterialCount == 0)
		{
			m_logger.Error("config.maxTextureCount must be > 0");
			return false;
		}

		m_config = config;

		m_materialShaderId = INVALID_ID;
		m_materialLocations.diffuseColor = INVALID_ID_U16;
		m_materialLocations.diffuseTexture = INVALID_ID_U16;

		m_uiShaderId = INVALID_ID;
		m_uiLocations.diffuseColor = INVALID_ID_U16;
		m_uiLocations.diffuseTexture = INVALID_ID_U16;

		// Allocate enough memory for the max number of materials that we will be using
		m_registeredMaterials = Memory.Allocate<Material>(config.maxMaterialCount, MemoryType::MaterialInstance);
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
			m_logger.Error("Failed to create default material");
			return false;
		}

		m_initialized = true;
		return true;
	}

	void MaterialSystem::Shutdown()
	{
		m_logger.Info("Destroying all loaded materials");
		for (u32 i = 0; i < m_config.maxMaterialCount; i++)
		{
			if (m_registeredMaterials[i].id != INVALID_ID)
			{
				DestroyMaterial(&m_registeredMaterials[i]);
			}
		}

		m_logger.Info("Destroying default material");
		DestroyMaterial(&m_defaultMaterial);

		// Free the memory we allocated for all our materials
		Memory.Free(m_registeredMaterials, sizeof(Material) * m_config.maxMaterialCount, MemoryType::MaterialInstance);
		m_registeredMaterials = nullptr;
	}

	Material* MaterialSystem::Acquire(const string& name)
	{
		Resource materialResource{};
		if (!Resources.Load(name, ResourceType::Material, &materialResource))
		{
			m_logger.Error("Failed to load material resource. Returning nullptr");
			return nullptr;
		}

		Material* m = nullptr;
		if (materialResource.data)
		{
			m = AcquireFromConfig(*materialResource.GetData<MaterialConfig*>());
		}

		Resources.Unload(&materialResource);

		if (!m)
		{
			m_logger.Error("Failed to load material resource. Returning nullptr");
		}
		return m;
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
				m_logger.Fatal("No more free space for materials. Adjust the configuration to allow more");
				return nullptr;
			}

			// Create a new Material
			if (!LoadMaterial(config, mat))
			{
				m_logger.Error("Failed to load material {}", config.name);
				return nullptr;
			}

			// Get the uniform indices
			Shader* shader = Shaders.GetById(mat->shaderId);
			// Save locations to known types for quick lookups
			if (m_materialShaderId == INVALID_ID && Equals(config.shaderName, BUILTIN_SHADER_NAME_MATERIAL))
			{
				m_materialShaderId = shader->id;
				m_materialLocations.projection = Shaders.GetUniformIndex(shader, "projection");
				m_materialLocations.view = Shaders.GetUniformIndex(shader, "view");
				m_materialLocations.ambientColor = Shaders.GetUniformIndex(shader, "ambientColor");
				m_materialLocations.diffuseColor = Shaders.GetUniformIndex(shader, "diffuseColor");
				m_materialLocations.shininess = Shaders.GetUniformIndex(shader, "shininess");
				m_materialLocations.viewPosition = Shaders.GetUniformIndex(shader, "viewPosition");
				m_materialLocations.diffuseTexture = Shaders.GetUniformIndex(shader, "diffuseTexture");
				m_materialLocations.specularTexture = Shaders.GetUniformIndex(shader, "specularTexture");
				m_materialLocations.normalTexture = Shaders.GetUniformIndex(shader, "normalTexture");
				m_materialLocations.model = Shaders.GetUniformIndex(shader, "model");
			}
			else if (m_uiShaderId == INVALID_ID && Equals(config.shaderName, BUILTIN_SHADER_NAME_UI))
			{
				m_uiShaderId = shader->id;
				m_uiLocations.projection = Shaders.GetUniformIndex(shader, "projection");
				m_uiLocations.view = Shaders.GetUniformIndex(shader, "view");
				m_uiLocations.diffuseColor = Shaders.GetUniformIndex(shader, "diffuseColor");
				m_uiLocations.diffuseTexture = Shaders.GetUniformIndex(shader, "diffuseTexture");
				m_uiLocations.model = Shaders.GetUniformIndex(shader, "model");
			}


			if (mat->generation == INVALID_ID) mat->generation = 0;
			else mat->generation++;

			mat->id = ref.handle;
			m_logger.Trace("Material {} did not exist yet. Created and the refCount is now {}", config.name, ref.referenceCount);
		}
		else
		{
			m_logger.Trace("Material {} already exists. The refCount is now {}", config.name, ref.referenceCount);
		}

		return &m_registeredMaterials[ref.handle];
	}

	void MaterialSystem::Release(const string& name)
	{
		if (IEquals(name, DEFAULT_MATERIAL_NAME))
		{
			m_logger.Warn("Tried to release {}. This happens automatically on shutdown", DEFAULT_MATERIAL_NAME);
			return;
		}

		const auto it = m_registeredMaterialTable.find(name);
		if (it == m_registeredMaterialTable.end())
		{
			m_logger.Warn("Tried to release a material that does not exist: {}", name);
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

			m_logger.Info("Released material {}. The texture was unloaded because refCount = 0 and autoRelease = true", name);
			return;
		}

		m_logger.Info("Released material {}. The material now has a refCount = {} (autoRelease = {})", name, ref.referenceCount, ref.autoRelease);
	}

	Material* MaterialSystem::GetDefault()
	{
		if (!m_initialized)
		{
			m_logger.Fatal("Tried to get the default material before system is initialized");
			return nullptr;
		}
		return &m_defaultMaterial;
	}

#define MATERIAL_APPLY_OR_FAIL(expr)					\
	if (!(expr))										\
	{													\
		m_logger.Error("Failed to apply: {}", expr);	\
		return false;									\
	}

	bool MaterialSystem::ApplyGlobal(u32 shaderId, const mat4* projection, const mat4* view, const vec4* ambientColor, const vec3* viewPosition) const
	{
		if (shaderId == m_materialShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.projection, projection))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.view, view))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.ambientColor, ambientColor))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.viewPosition, viewPosition))
		}
		else if (shaderId == m_uiShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.projection, projection))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.view, view))
		}
		else
		{
			m_logger.Error("ApplyGlobal() - Unrecognized shader id '{}'.", shaderId);
			return false;
		}

		MATERIAL_APPLY_OR_FAIL(Shaders.ApplyGlobal())
		return true;
	}

	bool MaterialSystem::ApplyInstance(Material* material) const
	{
		MATERIAL_APPLY_OR_FAIL(Shaders.BindInstance(material->internalId))
		if (material->shaderId == m_materialShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.diffuseColor, &material->diffuseColor))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.shininess, &material->shininess))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.diffuseTexture, material->diffuseMap.texture))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.specularTexture, material->specularMap.texture))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.normalTexture, material->normalMap.texture))
		}
		else if (material->shaderId == m_uiShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.diffuseColor, &material->diffuseColor))
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.diffuseTexture, material->diffuseMap.texture))
		}
		else
		{
			m_logger.Error("ApplyInstance() - Unrecognized shader id '{}' on material: '{}'.", material->shaderId, material->name);
			return false;
		}

		MATERIAL_APPLY_OR_FAIL(Shaders.ApplyInstance())
		return true;
	}

	bool MaterialSystem::ApplyLocal(Material* material, const mat4* model) const
	{
		if (material->shaderId == m_materialShaderId)
		{
			return Shaders.SetUniformByIndex(m_materialLocations.model, model);
		}
		if (material->shaderId == m_uiShaderId)
		{
			return Shaders.SetUniformByIndex(m_uiLocations.model, model);
		}

		m_logger.Error("ApplyLocal() - Unrecognized shader id: '{}' on material: '{}'.", material->shaderId, material->name);
		return false;
	}

	bool MaterialSystem::CreateDefaultMaterial()
	{
		Memory.Zero(&m_defaultMaterial, sizeof(Material));

		m_defaultMaterial.id = INVALID_ID;
		m_defaultMaterial.generation = INVALID_ID;
		StringNCopy(m_defaultMaterial.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
		m_defaultMaterial.diffuseColor = vec4(1);

		m_defaultMaterial.diffuseMap.use = TextureUse::Diffuse;
		m_defaultMaterial.diffuseMap.texture = Textures.GetDefault();

		m_defaultMaterial.specularMap.use = TextureUse::Specular;
		m_defaultMaterial.specularMap.texture = Textures.GetDefaultSpecular();

		m_defaultMaterial.normalMap.use = TextureUse::Normal;
		m_defaultMaterial.normalMap.texture = Textures.GetDefaultNormal();

		Shader* shader = Shaders.Get(BUILTIN_SHADER_NAME_MATERIAL);
		if (!Renderer.AcquireShaderInstanceResources(shader, &m_defaultMaterial.internalId))
		{
			m_logger.Error("Failed to acquire renderer resources for the default material");
			return false;
		}

		// Assign the shader id to the default material
		m_defaultMaterial.shaderId = shader->id;
		return true;
	}

	bool MaterialSystem::LoadMaterial(const MaterialConfig& config, Material* mat) const
	{
		Memory.Zero(mat, sizeof(Material));

		// Name
		StringNCopy(mat->name, config.name, MATERIAL_NAME_MAX_LENGTH);
		// Id of the shader associated with this material
		mat->shaderId = Shaders.GetId(config.shaderName);
		// Diffuse color
		mat->diffuseColor = config.diffuseColor;
		// Shininess
		mat->shininess = config.shininess;

		// Diffuse map
		if (StringLength(config.diffuseMapName) > 0)
		{
			mat->diffuseMap.use = TextureUse::Diffuse;
			mat->diffuseMap.texture = Textures.Acquire(config.diffuseMapName, true);

			if (!mat->diffuseMap.texture)
			{
				m_logger.Warn("Unable to load diffuse texture '{}' for material '{}', using the default", config.diffuseMapName, mat->name);
				mat->diffuseMap.texture = Textures.GetDefault();
			}
		}
		else
		{
			// NOTE: this does nothing since we already zeroed the memory. It's only for clarity
			mat->diffuseMap.use = TextureUse::Diffuse;
			mat->diffuseMap.texture = Textures.GetDefault();
		}

		// Specular  map
		if (StringLength(config.specularMapName) > 0)
		{
			mat->specularMap.use = TextureUse::Specular;
			mat->specularMap.texture = Textures.Acquire(config.specularMapName, true);

			if (!mat->specularMap.texture)
			{
				m_logger.Warn("Unable to load specular texture '{}' for material '{}', using the default", config.specularMapName, mat->name);
				mat->specularMap.texture = Textures.GetDefaultSpecular();
			}
		}
		else
		{
			// NOTE: this does nothing since we already zeroed the memory. It's only for clarity
			mat->specularMap.use = TextureUse::Specular;
			mat->specularMap.texture = Textures.GetDefaultSpecular();
		}

		// Normal  map
		if (StringLength(config.normalMapName) > 0)
		{
			mat->normalMap.use = TextureUse::Normal;
			mat->normalMap.texture = Textures.Acquire(config.normalMapName, true);

			if (!mat->normalMap.texture)
			{
				m_logger.Warn("Unable to load normal texture '{}' for material '{}', using the default", config.normalMapName, mat->name);
				mat->normalMap.texture = Textures.GetDefaultNormal();
			}
		}
		else
		{
			mat->normalMap.use = TextureUse::Normal;
			mat->normalMap.texture = Textures.GetDefaultNormal();
		}

		Shader* shader = Shaders.Get(config.shaderName);
		if (!Renderer.AcquireShaderInstanceResources(shader, &mat->internalId))
		{
			m_logger.Error("Failed to acquire renderer resources for material: {}", mat->name);
			return false;
		}

		return true;
	}

	void MaterialSystem::DestroyMaterial(Material* mat) const
	{
		m_logger.Trace("Destroying material '{}'", mat->name);

		// If the diffuseMap has a texture we release it
		if (mat->diffuseMap.texture)
		{
			Textures.Release(mat->diffuseMap.texture->name);
		}

		// If the specularMap has a texture we release it
		if (mat->specularMap.texture)
		{
			Textures.Release(mat->specularMap.texture->name);
		}

		// If the normalMap has a texture we release it
		if (mat->normalMap.texture)
		{
			Textures.Release(mat->normalMap.texture->name);
		}

		// Release renderer resources
		if (mat->shaderId != INVALID_ID && mat->internalId != INVALID_ID)
		{
			Shader* shader = Shaders.GetById(mat->shaderId);
			Renderer.ReleaseShaderInstanceResources(shader, mat->internalId);
			mat->shaderId = INVALID_ID;
		}

		// Zero out memory and invalidate ids
		Memory.Zero(mat, sizeof(Material));
		mat->id = INVALID_ID;
		mat->generation = INVALID_ID;
		mat->internalId = INVALID_ID;
	}
}
