
#include "material_system.h"

#include "core/engine.h"
#include "core/logger.h"
#include "core/string_utils.h"

#include "renderer/renderer_types.h"
#include "renderer/renderer_frontend.h"
#include "resources/shader.h"
#include "resources/loaders/material_loader.h"

#include "systems/system_manager.h"
#include "systems/shaders/shader_system.h"
#include "systems/resources/resource_system.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
	MaterialSystem::MaterialSystem(const Engine* engine)
		: SystemWithConfig(engine, "MATERIAL_SYSTEM"), m_initialized(false), m_defaultMaterial(), m_registeredMaterials(nullptr),
		  m_materialShaderId(0), m_uiShaderId(0)
	{
	}

	bool MaterialSystem::Init(const MaterialSystemConfig& config)
	{
		if (config.maxMaterialCount == 0)
		{
			m_logger.Error("config.maxTextureCount must be > 0");
			return false;
		}

		m_config = config;

		m_materialShaderId = INVALID_ID;
		m_uiShaderId = INVALID_ID;

		// Allocate enough memory for the max number of materials that we will be using
		m_registeredMaterials = Memory.Allocate<Material>(MemoryType::MaterialInstance, config.maxMaterialCount);
		// Set the id of all the textures to invalid
		for (u32 i = 0; i < m_config.maxMaterialCount; i++)
		{
			m_registeredMaterials[i].id = INVALID_ID;
			m_registeredMaterials[i].generation = INVALID_ID;
			m_registeredMaterials[i].internalId = INVALID_ID;
			m_registeredMaterials[i].renderFrameNumber = INVALID_ID;
		}

		// Ensure that we have enough space for all our textures
		m_registeredMaterialTable.Create(config.maxMaterialCount);
		m_registeredMaterialTable.FillDefault();

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
		Memory.Free(MemoryType::MaterialInstance, m_registeredMaterials);
		m_registeredMaterials = nullptr;

		// Cleanup our material table
		m_registeredMaterialTable.Destroy();
	}

	Material* MaterialSystem::Acquire(const char* name)
	{
		MaterialResource materialResource{};
		if (!Resources.Load(name, materialResource))
		{
			m_logger.Error("Failed to load material resource. Returning nullptr");
			return nullptr;
		}

		Material* m = AcquireFromConfig(materialResource.config);
		Resources.Unload(materialResource);

		if (!m)
		{
			m_logger.Error("Failed to load material resource. Returning nullptr");
		}
		return m;
	}

	Material* MaterialSystem::AcquireFromConfig(const MaterialConfig& config)
	{
		if (config.name.IEquals(DEFAULT_MATERIAL_NAME))
		{
			return &m_defaultMaterial;
		}

		MaterialReference ref = m_registeredMaterialTable.Get(config.name);

		// This should only happen the first time a material is loaded
		if (ref.referenceCount == 0)
		{
			ref.autoRelease = config.autoRelease;
		}

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
			if (m_materialShaderId == INVALID_ID && config.shaderName == "Shader.Builtin.Material")
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
				m_materialLocations.renderMode = Shaders.GetUniformIndex(shader, "mode");
			}
			else if (m_uiShaderId == INVALID_ID && config.shaderName == "Shader.Builtin.UI")
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

			// Use the handle (location in the array) as the material id
			mat->id = ref.handle;
			m_logger.Trace("Material {} did not exist yet. Created and the refCount is now {}", config.name, ref.referenceCount);
		}
		else
		{
			m_logger.Trace("Material {} already exists. The refCount is now {}", config.name, ref.referenceCount);
		}

		m_registeredMaterialTable.Set(config.name, ref);
		return &m_registeredMaterials[ref.handle];
	}

	void MaterialSystem::Release(const char* name)
	{
		if (StringUtils::IEquals(name, DEFAULT_MATERIAL_NAME))
		{
			m_logger.Warn("Tried to release {}. This happens automatically on shutdown", DEFAULT_MATERIAL_NAME);
			return;
		}

		MaterialReference ref = m_registeredMaterialTable.Get(name);
		if (ref.referenceCount == 0)
		{
			m_logger.Warn("Tried to release a material that does not exist: {}", name);
			return;
		}

		ref.referenceCount--;
		if (ref.referenceCount == 0 && ref.autoRelease)
		{
			// This material is marked for auto release and we are holding no more references to it
			Material* mat = &m_registeredMaterials[ref.handle];

			// Destroy the material
			DestroyMaterial(mat);

			// Reset the reference
			ref.handle = INVALID_ID;
			ref.autoRelease = false;

			m_logger.Info("Released material {}. The texture was unloaded because refCount = 0 and autoRelease = true", name);
		}
		else
		{
			m_logger.Info("Released material {}. The material now has a refCount = {} (autoRelease = {})", name, ref.referenceCount, ref.autoRelease);
		}

		m_registeredMaterialTable.Set(name, ref);
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

	bool MaterialSystem::ApplyGlobal(u32 shaderId, u64 frameNumber, const mat4* projection, const mat4* view, const vec4* ambientColor, const vec3* viewPosition, const u32 renderMode) const
	{
		Shader* s = Shaders.GetById(shaderId);
		if (!s)
		{
			m_logger.Error("No Shader found with id '{}'.", shaderId);
			return false;
		}
		if (s->frameNumber == frameNumber)
		{
			// The globals have already been applied for this frame so we don't need to do anything here.
			return true;
		}

		if (shaderId == m_materialShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.projection, projection));
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.view, view));
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.ambientColor, ambientColor));
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.viewPosition, viewPosition));
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.renderMode, &renderMode));
		}
		else if (shaderId == m_uiShaderId)
		{
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.projection, projection));
			MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.view, view));
		}
		else
		{
			m_logger.Error("ApplyGlobal() - Unrecognized shader id '{}'.", shaderId);
			return false;
		}

		MATERIAL_APPLY_OR_FAIL(Shaders.ApplyGlobal())

		// Sync the frameNumber
		s->frameNumber = frameNumber;
		return true;
	}

	bool MaterialSystem::ApplyInstance(Material* material, const bool needsUpdate) const
	{
		MATERIAL_APPLY_OR_FAIL(Shaders.BindInstance(material->internalId))
		if (needsUpdate)
		{
			if (material->shaderId == m_materialShaderId)
			{
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.diffuseColor, &material->diffuseColor));
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.shininess, &material->shininess));
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.diffuseTexture, &material->diffuseMap));
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.specularTexture, &material->specularMap));
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.normalTexture, &material->normalMap));
			}
			else if (material->shaderId == m_uiShaderId)
			{
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.diffuseColor, &material->diffuseColor));
				MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_uiLocations.diffuseTexture, &material->diffuseMap));
			}
			else
			{
				m_logger.Error("ApplyInstance() - Unrecognized shader id '{}' on material: '{}'.", material->shaderId, material->name);
				return false;
			}
		}

		MATERIAL_APPLY_OR_FAIL(Shaders.ApplyInstance(needsUpdate))
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
		Platform::Zero(&m_defaultMaterial);

		m_defaultMaterial.id = INVALID_ID;
		m_defaultMaterial.generation = INVALID_ID;
		m_defaultMaterial.name = DEFAULT_MATERIAL_NAME;
		m_defaultMaterial.diffuseColor = vec4(1);

		m_defaultMaterial.diffuseMap.use = TextureUse::Diffuse;
		m_defaultMaterial.diffuseMap.texture = Textures.GetDefaultDiffuse();

		m_defaultMaterial.specularMap.use = TextureUse::Specular;
		m_defaultMaterial.specularMap.texture = Textures.GetDefaultSpecular();

		m_defaultMaterial.normalMap.use = TextureUse::Normal;
		m_defaultMaterial.normalMap.texture = Textures.GetDefaultNormal();

		TextureMap* maps[3] = { &m_defaultMaterial.diffuseMap, &m_defaultMaterial.specularMap, &m_defaultMaterial.normalMap };

		Shader* shader = Shaders.Get("Shader.Builtin.Material");
		if (!Renderer.AcquireShaderInstanceResources(shader, maps, &m_defaultMaterial.internalId))
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
		Platform::Zero(mat);

		// Name
		mat->name = config.name;
		// Id of the shader associated with this material
		mat->shaderId = Shaders.GetId(config.shaderName.Data());
		// Diffuse color
		mat->diffuseColor = config.diffuseColor;
		// Shininess
		mat->shininess = config.shininess;

		// Diffuse map
		// TODO: make this configurable
		mat->diffuseMap = TextureMap(TextureUse::Diffuse, TextureFilter::ModeLinear, TextureRepeat::Repeat);
		if (!Renderer.AcquireTextureMapResources(&mat->diffuseMap))
		{
			m_logger.Error("LoadMaterial() - Unable to acquire resources for diffuse texture map");
			return false;
		}

		if (!config.diffuseMapName.Empty())
		{
			mat->diffuseMap.texture = Textures.Acquire(config.diffuseMapName.Data(), true);
			if (!mat->diffuseMap.texture)
			{
				m_logger.Warn("Unable to load diffuse texture '{}' for material '{}', using the default", config.diffuseMapName, mat->name);
				mat->diffuseMap.texture = Textures.GetDefaultDiffuse();
			}
		}
		else
		{
			mat->diffuseMap.texture = Textures.GetDefaultDiffuse();
		}

		// Specular  map
		// TODO: make this configurable
		mat->specularMap = TextureMap(TextureUse::Specular, TextureFilter::ModeLinear, TextureRepeat::Repeat);
		if (!Renderer.AcquireTextureMapResources(&mat->specularMap))
		{
			m_logger.Error("LoadMaterial() - Unable to acquire resources for diffuse texture map");
			return false;
		}

		if (!config.specularMapName.Empty())
		{
			mat->specularMap.texture = Textures.Acquire(config.specularMapName.Data(), true);

			if (!mat->specularMap.texture)
			{
				m_logger.Warn("Unable to load specular texture '{}' for material '{}', using the default", config.specularMapName, mat->name);
				mat->specularMap.texture = Textures.GetDefaultSpecular();
			}
		}
		else
		{
			mat->specularMap.texture = Textures.GetDefaultSpecular();
		}

		// Normal  map
		// TODO: make this configurable
		mat->normalMap = TextureMap(TextureUse::Normal, TextureFilter::ModeLinear, TextureRepeat::Repeat);
		if (!Renderer.AcquireTextureMapResources(&mat->normalMap))
		{
			m_logger.Error("LoadMaterial() - Unable to acquire resources for diffuse texture map");
			return false;
		}

		if (!config.normalMapName.Empty())
		{
			mat->normalMap.texture = Textures.Acquire(config.normalMapName.Data(), true);

			if (!mat->normalMap.texture)
			{
				m_logger.Warn("Unable to load normal texture '{}' for material '{}', using the default", config.normalMapName, mat->name);
				mat->normalMap.texture = Textures.GetDefaultNormal();
			}
		}
		else
		{
			mat->normalMap.texture = Textures.GetDefaultNormal();
		}

		Shader* shader = Shaders.Get(config.shaderName.Data());
		if (!shader)
		{
			m_logger.Error("LoadMaterial() - Failed to load material since it's shader was not found: '{}'", config.shaderName);
			return false;
		}

		// Gather a list of pointers to our texture maps
		TextureMap* maps[3] = { &mat->diffuseMap, &mat->specularMap, &mat->normalMap };
		if (!Renderer.AcquireShaderInstanceResources(shader, maps, &mat->internalId))
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
			Textures.Release(mat->diffuseMap.texture->name.Data());
		}

		// If the specularMap has a texture we release it
		if (mat->specularMap.texture)
		{
			Textures.Release(mat->specularMap.texture->name.Data());
		}

		// If the normalMap has a texture we release it
		if (mat->normalMap.texture)
		{
			Textures.Release(mat->normalMap.texture->name.Data());
		}

		// Release texture map resources
		Renderer.ReleaseTextureMapResources(&mat->diffuseMap);
		Renderer.ReleaseTextureMapResources(&mat->specularMap);
		Renderer.ReleaseTextureMapResources(&mat->normalMap);

		// Release renderer resources
		if (mat->shaderId != INVALID_ID && mat->internalId != INVALID_ID)
		{
			Shader* shader = Shaders.GetById(mat->shaderId);
			Renderer.ReleaseShaderInstanceResources(shader, mat->internalId);
			mat->shaderId = INVALID_ID;
		}

		// Zero out memory and invalidate ids
		Platform::Zero(mat);
		mat->id = INVALID_ID;
		mat->generation = INVALID_ID;
		mat->internalId = INVALID_ID;
		mat->renderFrameNumber = INVALID_ID;
	}
}
