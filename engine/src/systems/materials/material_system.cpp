
#include "material_system.h"

#include "core/clock.h"
#include "core/engine.h"
#include "core/frame_data.h"
#include "core/logger.h"
#include "core/string_utils.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_types.h"
#include "resources/managers/material_manager.h"
#include "resources/shaders/shader.h"
#include "systems/cvars/cvar_system.h"
#include "systems/lights/light_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    bool MaterialSystem::OnInit(const MaterialSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxMaterials == 0)
        {
            ERROR_LOG("config.maxTextureCount must be > 0.");
            return false;
        }

        m_config = config;

        // Reserve enough space to store our materials
        m_materials.Reserve(config.maxMaterials);

        // Create our HashMap to store Name to index mappings
        m_nameToMaterialIndexMap.Create();

        // Get the uniform indices and save them off for quick lookups
        // Start with the PBR shader
        Shader* shader                  = Shaders.Get("Shader.PBR");
        m_pbrShaderId                   = shader->id;
        m_pbrLocations.projection       = Shaders.GetUniformIndex(shader, "projection");
        m_pbrLocations.view             = Shaders.GetUniformIndex(shader, "view");
        m_pbrLocations.lightSpaces      = Shaders.GetUniformIndex(shader, "lightSpaces");
        m_pbrLocations.cascadeSplits    = Shaders.GetUniformIndex(shader, "cascadeSplits");
        m_pbrLocations.viewPosition     = Shaders.GetUniformIndex(shader, "viewPosition");
        m_pbrLocations.properties       = Shaders.GetUniformIndex(shader, "properties");
        m_pbrLocations.iblCubeTexture   = Shaders.GetUniformIndex(shader, "iblCubeTexture");
        m_pbrLocations.materialTextures = Shaders.GetUniformIndex(shader, "materialTextures");
        m_pbrLocations.shadowTextures   = Shaders.GetUniformIndex(shader, "shadowTextures");
        m_pbrLocations.model            = Shaders.GetUniformIndex(shader, "model");
        m_pbrLocations.renderMode       = Shaders.GetUniformIndex(shader, "mode");
        m_pbrLocations.dirLight         = Shaders.GetUniformIndex(shader, "dirLight");
        m_pbrLocations.pLights          = Shaders.GetUniformIndex(shader, "pLights");
        m_pbrLocations.numPLights       = Shaders.GetUniformIndex(shader, "numPLights");
        m_pbrLocations.usePCF           = Shaders.GetUniformIndex(shader, "usePCF");
        m_pbrLocations.bias             = Shaders.GetUniformIndex(shader, "bias");

        // Finally the Terrain Shader
        shader                              = Shaders.Get("Shader.Builtin.Terrain");
        m_terrainShaderId                   = shader->id;
        m_terrainLocations.projection       = Shaders.GetUniformIndex(shader, "projection");
        m_terrainLocations.view             = Shaders.GetUniformIndex(shader, "view");
        m_terrainLocations.lightSpaces      = Shaders.GetUniformIndex(shader, "lightSpaces");
        m_terrainLocations.cascadeSplits    = Shaders.GetUniformIndex(shader, "cascadeSplits");
        m_terrainLocations.viewPosition     = Shaders.GetUniformIndex(shader, "viewPosition");
        m_terrainLocations.model            = Shaders.GetUniformIndex(shader, "model");
        m_terrainLocations.renderMode       = Shaders.GetUniformIndex(shader, "mode");
        m_terrainLocations.dirLight         = Shaders.GetUniformIndex(shader, "dirLight");
        m_terrainLocations.pLights          = Shaders.GetUniformIndex(shader, "pLights");
        m_terrainLocations.numPLights       = Shaders.GetUniformIndex(shader, "numPLights");
        m_terrainLocations.properties       = Shaders.GetUniformIndex(shader, "properties");
        m_terrainLocations.materialTextures = Shaders.GetUniformIndex(shader, "materialTextures");
        m_terrainLocations.shadowTextures   = Shaders.GetUniformIndex(shader, "shadowTextures");
        m_terrainLocations.iblCubeTexture   = Shaders.GetUniformIndex(shader, "iblCubeTexture");
        m_terrainLocations.usePCF           = Shaders.GetUniformIndex(shader, "usePCF");
        m_terrainLocations.bias             = Shaders.GetUniformIndex(shader, "bias");

        if (!CreateDefaultPbrMaterial())
        {
            ERROR_LOG("Failed to create Default PBR Material.");
            return false;
        }

        if (!CreateDefaultTerrainMaterial())
        {
            ERROR_LOG("Failed to create Default Terrain Material.");
            return false;
        }

        // Set our current irradiance texture to the default
        m_currentIrradianceTexture = Textures.GetDefaultCube();
        // Set our current shadow texture to default diffuse
        m_currentShadowTexture = Textures.GetDefaultAlbedo();

        if (!CVars.Create("usePCF", m_usePCF, [this](const CVar& cvar) { m_usePCF = cvar.GetValue<i32>(); }))
        {
            ERROR_LOG("Failed to create usePCF CVar.");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void MaterialSystem::OnShutdown()
    {
        INFO_LOG("Destroying all loaded materials.");
        for (auto& ref : m_materials)
        {
            if (ref.material.id != INVALID_ID)
            {
                DestroyMaterial(ref.material);
            }
        }

        // Destroy our Material array and our HashMap
        m_materials.Destroy();
        m_nameToMaterialIndexMap.Destroy();

        INFO_LOG("Destroying default materials.");
        DestroyMaterial(m_defaultTerrainMaterial);
        DestroyMaterial(m_defaultPbrMaterial);
    }

    Material* MaterialSystem::Acquire(const String& name)
    {
        if (m_nameToMaterialIndexMap.Has(name))
        {
            // The material already exists
            auto index             = m_nameToMaterialIndexMap.Get(name);
            MaterialReference& ref = m_materials[index];
            ref.referenceCount++;

            TRACE("Material: '{}' already exists. The refCount is now: {}.", name, ref.referenceCount);

            return &ref.material;
        }

        MaterialConfig materialConfig;
        if (!Resources.Read(name, materialConfig))
        {
            ERROR_LOG("Failed to load material resource: '{}'. Returning nullptr.", name);
            return nullptr;
        }

        Material* m = AcquireFromConfig(materialConfig);
        Resources.Cleanup(materialConfig);

        if (!m)
        {
            ERROR_LOG("Failed to acquire material from config: '{}'. Returning nullptr.", name);
        }
        return m;
    }

    Material& MaterialSystem::AcquireReference(const String& name, bool autoRelease, bool& needsCreation)
    {
        if (m_nameToMaterialIndexMap.Has(name))
        {
            // The material already exists
            auto index             = m_nameToMaterialIndexMap.Get(name);
            MaterialReference& ref = m_materials[index];
            ref.referenceCount++;

            TRACE("Material: '{}' already exists. The refCount is now: {}.", name, ref.referenceCount);

            needsCreation = false;
            return ref.material;
        }

        // The material does not exist yet
        // Add a new reference into the materials array and add it to our HashMap
        u32 index = INVALID_ID;
        for (u32 i = 0; i < m_materials.Size(); ++i)
        {
            auto& current = m_materials[i];

            if (current.material.id == INVALID_ID)
            {
                index   = i;
                current = MaterialReference(autoRelease, i);
            }
        }

        if (index == INVALID_ID)
        {
            // We did not find an empty spot in our array so let's append
            index = m_materials.Size();
            m_materials.EmplaceBack(autoRelease, index);
        }

        // Store the name and index combination in our HashMap
        m_nameToMaterialIndexMap.Set(name, index);
        // Mark that this material still needs to be created
        needsCreation = true;
        // Return a reference to the material
        return m_materials[index].material;
    }

    Material* MaterialSystem::AcquireTerrain(const String& name, const DynamicArray<String>& materialNames, bool autoRelease)
    {
        // Return the Default Terrain Material
        if (name.IEquals(DEFAULT_TERRAIN_MATERIAL_NAME))
        {
            return &m_defaultTerrainMaterial;
        }

        bool needsCreation = false;

        Material& mat = AcquireReference(name, autoRelease, needsCreation);
        if (needsCreation)
        {
            // Loop over all materials that we want to include in this terrain and load their config
            // Then extract the individual texture names from each material and collect them
            DynamicArray<String> textureNames(materialNames.Size() * PBR_MATERIAL_TEXTURE_COUNT);

            for (const auto& name : materialNames)
            {
                // Load each individual material config
                MaterialConfig config;
                if (!Resources.Read(name, config))
                {
                    ERROR_LOG("Failed to load material resources: '{}'.", name)
                    return nullptr;
                }

                // Only PBR materials are supported
                if (config.type != MaterialType::PBR)
                {
                    ERROR_LOG("Material: '{}' is not of type PBR which is required for terrains.", config.name);
                    return nullptr;
                }

                // Extract the texture names from the maps in the material
                for (auto& map : config.maps)
                {
                    textureNames.PushBack(map.textureName);
                }

                // Cleanup our config
                Resources.Cleanup(config);
            }

            // Create a new Terrain Material that will hold all these internal materials
            mat.name = name;

            Shader* shader = Shaders.Get("Shader.Builtin.Terrain");
            mat.shaderId   = shader->id;
            mat.type       = MaterialType::Terrain;

            // Allocate space for the properties
            mat.propertiesSize = sizeof(MaterialTerrainProperties);
            mat.properties     = Memory.Allocate<MaterialTerrainProperties>(MemoryType::MaterialInstance);

            auto props          = static_cast<MaterialTerrainProperties*>(mat.properties);
            props->padding      = vec3(0);
            props->numMaterials = materialNames.Size();
            props->padding2     = vec4(0);

            // Array texture for all material textures, array texture for shadow maps and a texture for the irradiance map
            mat.maps.Resize(TERRAIN_TOTAL_MAP_COUNT);

            {
                // First we load the material textures into one big array texture
                auto& map = mat.maps[TERRAIN_SAMP_MATERIALS];
                // For every terrain material we have 4 PBR materials (3 maps per material)
                u32 layerCount = PBR_MATERIAL_TEXTURE_COUNT * TERRAIN_MAX_MATERIAL_COUNT;
                map.texture    = Textures.AcquireArray(mat.name.Data(), layerCount, textureNames, true);
                if (!map.texture)
                {
                    WARN_LOG("Unable to load Array texture for terrain: '{}'. Using default texture.", name);
                    map.texture = Textures.GetDefaultTerrain();
                }
                if (!Renderer.AcquireTextureMapResources(map))
                {
                    ERROR_LOG("Failed to acquire resources for texture map.");
                    return nullptr;
                }
            }

            // Cleanup our texture names
            textureNames.Destroy();

            // Shadow map
            {
                MaterialConfigMap mapConfig;
                mapConfig.name          = "shadowMap";
                mapConfig.repeatU       = TextureRepeat::ClampToBorder;
                mapConfig.repeatV       = TextureRepeat::ClampToBorder;
                mapConfig.repeatW       = TextureRepeat::ClampToBorder;
                mapConfig.minifyFilter  = TextureFilter::ModeLinear;
                mapConfig.magnifyFilter = TextureFilter::ModeLinear;
                mapConfig.textureName   = "";
                if (!AssignMap(mat.maps[TERRAIN_SAMP_SHADOW_MAP], mapConfig, Textures.GetDefaultDiffuse()))
                {
                    ERROR_LOG("Failed to assign: {} texture map for terrain shadow map.", mapConfig.name);
                    return nullptr;
                }
            }

            // IBL - cubemap for irradiance
            {
                MaterialConfigMap mapConfig;
                mapConfig.name          = "iblCube";
                mapConfig.repeatU       = TextureRepeat::Repeat;
                mapConfig.repeatV       = TextureRepeat::Repeat;
                mapConfig.repeatW       = TextureRepeat::Repeat;
                mapConfig.minifyFilter  = TextureFilter::ModeLinear;
                mapConfig.magnifyFilter = TextureFilter::ModeLinear;
                mapConfig.textureName   = "";
                if (!AssignMap(mat.maps[TERRAIN_SAMP_IRRADIANCE_MAP], mapConfig, Textures.GetDefaultCube()))
                {
                    ERROR_LOG("Failed to assign: {} texture map for terrain irradiance map.", mapConfig.name);
                    return nullptr;
                }
            }

            // Acquire instance resource for all our maps
            ShaderInstanceResourceConfig instanceConfig;
            // 3 configs: material maps, shadows maps and irradiance map
            instanceConfig.uniformConfigCount = 3;
            instanceConfig.uniformConfigs =
                Memory.Allocate<ShaderInstanceUniformTextureConfig>(MemoryType::Array, instanceConfig.uniformConfigCount);

            // Material textures (1 array texture of MATERIAL_COUNT * PBR_MATERIAL_TEXTURE_COUNT textures)
            auto& matTextures           = instanceConfig.uniformConfigs[0];
            matTextures.uniformLocation = m_terrainLocations.materialTextures;
            matTextures.textureMapCount = 1;
            matTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, matTextures.textureMapCount);
            matTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_MATERIALS];

            // Shadow textures
            auto& shadowTextures           = instanceConfig.uniformConfigs[1];
            shadowTextures.uniformLocation = m_terrainLocations.shadowTextures;
            shadowTextures.textureMapCount = 1;
            shadowTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, shadowTextures.textureMapCount);
            shadowTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_SHADOW_MAP];

            // Irradiance cube texture
            auto& iblTextures           = instanceConfig.uniformConfigs[2];
            iblTextures.uniformLocation = m_terrainLocations.iblCubeTexture;
            iblTextures.textureMapCount = 1;
            iblTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, iblTextures.textureMapCount);
            iblTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_IRRADIANCE_MAP];

            if (!Renderer.AcquireShaderInstanceResources(*shader, instanceConfig, mat.internalId))
            {
                ERROR_LOG("Failed to acquire renderer resources for material: '{}'.", mat.name);
            }

            // Free our uniform config texture maps
            for (u32 i = 0; i < instanceConfig.uniformConfigCount; i++)
            {
                auto& cfg = instanceConfig.uniformConfigs[i];
                Memory.Free(cfg.textureMaps);
                cfg.textureMaps = nullptr;
            }

            // Free our uniform configs array
            Memory.Free(instanceConfig.uniformConfigs);

            if (mat.generation == INVALID_ID)
            {
                mat.generation = 0;
            }
            else
            {
                mat.generation++;
            }
        }

        return &mat;
    }

    Material* MaterialSystem::AcquireFromConfig(const MaterialConfig& config)
    {
        // Return Default PBR Material
        if (config.name.IEquals(DEFAULT_PBR_MATERIAL_NAME))
        {
            return &m_defaultPbrMaterial;
        }

        // Return Default Terrain Material
        if (config.name.IEquals(DEFAULT_TERRAIN_MATERIAL_NAME))
        {
            return &m_defaultTerrainMaterial;
        }

        bool needsCreation = false;

        Material& mat = AcquireReference(config.name, config.autoRelease, needsCreation);
        if (needsCreation)
        {
            if (!LoadMaterial(config, mat))
            {
                ERROR_LOG("Failed to Load Material: '{}'.", config.name);
                return nullptr;
            }

            if (mat.generation == INVALID_ID)
            {
                mat.generation = 0;
            }
            else
            {
                mat.generation++;
            }
        }

        return &mat;
    }

    void MaterialSystem::Release(const String& name)
    {
        if (name.IEquals(DEFAULT_PBR_MATERIAL_NAME) || name.IEquals(DEFAULT_TERRAIN_MATERIAL_NAME))
        {
            WARN_LOG("Tried to release Default Material. This happens automatically on shutdown.");
            return;
        }

        if (!m_nameToMaterialIndexMap.Has(name))
        {
            WARN_LOG("Tried to release a material that does not exist: '{}'.", name);
            return;
        }

        auto index             = m_nameToMaterialIndexMap.Get(name);
        MaterialReference& ref = m_materials[index];
        ref.referenceCount--;

        if (ref.referenceCount == 0 && ref.autoRelease)
        {
            // This material is marked for auto release and we are holding no more references to it

            // Make a copy of the name in case the material's name itself is used for calling this method since
            // DestroyMaterial() will clear that name
            auto nameCopy = ref.material.name;

            // Destroy the material
            DestroyMaterial(ref.material);

            // Remove the material reference
            m_materials[index].material.id = INVALID_ID;
            // Also remove it from our name to index HashMap
            m_nameToMaterialIndexMap.Delete(nameCopy);

            TRACE("The Material: '{}' was released. The texture was unloaded because refCount = 0 and autoRelease = true.", nameCopy);
        }
        else
        {
            TRACE("The Material: '{}' now has a refCount = {} (autoRelease = {}).", name, ref.referenceCount, ref.autoRelease);
        }
    }

    bool MaterialSystem::SetIrradiance(TextureHandle handle)
    {
        if (handle == INVALID_ID)
        {
            ERROR_LOG("Invalid irradiance cube texture provided.");
            return false;
        }

        const auto& texture = Textures.Get(handle);

        if (texture.type != TextureTypeCube)
        {
            ERROR_LOG("Provied texture is not of type: TextureTypeCube.");
            return false;
        }

        m_currentIrradianceTexture = handle;
        return true;
    }

    void MaterialSystem::ResetIrradiance() { m_currentIrradianceTexture = Textures.GetDefaultCube(); }

    void MaterialSystem::SetShadowMap(TextureHandle handle, u8 cascadeIndex)
    {
        m_currentShadowTexture = (handle == INVALID_ID) ? Textures.GetDefaultAlbedo() : handle;
    }

    void MaterialSystem::SetDirectionalLightSpaceMatrix(const mat4& lightSpace, u8 cascadeIndex)
    {
        m_directionalLightSpace[cascadeIndex] = lightSpace;
    }

#define MATERIAL_APPLY_OR_FAIL(expr)               \
    if (!(expr))                                   \
    {                                              \
        ERROR_LOG("Failed to apply: {}.", (expr)); \
        return false;                              \
    }

    bool MaterialSystem::ApplyGlobal(u32 shaderId, const FrameData& frameData, const mat4* projection, const mat4* view,
                                     const vec4* cascadeSplits, const vec3* viewPosition, const u32 renderMode) const
    {
        Shader* s = Shaders.GetById(shaderId);
        if (!s)
        {
            ERROR_LOG("No Shader found with id: '{}'.", shaderId);
            return false;
        }
        if (s->frameNumber == frameData.frameNumber && s->drawIndex == frameData.drawIndex)
        {
            // The globals have already been applied for this frame so we don't need to do anything here.
            return true;
        }

        if (shaderId == m_terrainShaderId)
        {
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.projection, projection));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.view, view));
            // TODO: Set cascade splits like dir light and shadow map etc.
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.cascadeSplits, cascadeSplits));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.viewPosition, viewPosition));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.renderMode, &renderMode));
            // Light space for shadow mapping
            for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
            {
                MATERIAL_APPLY_OR_FAIL(Shaders.SetArrayUniformByIndex(m_terrainLocations.lightSpaces, i, &m_directionalLightSpace[i]));
            }

            // Directional light is global for terrains
            const auto dirLight = Lights.GetDirectionalLight();
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.dirLight, &dirLight->data));

            // Global shader options
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.usePCF, &m_usePCF));

            // HACK:
            f32 bias = 0.00005f;
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.bias, &bias));
        }
        else if (shaderId == m_pbrShaderId)
        {
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.projection, projection));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.view, view));
            // TODO: Set cascade splits like dir light and shadow map etc.
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.cascadeSplits, cascadeSplits))
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.viewPosition, viewPosition));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.renderMode, &renderMode));
            // Light space for shadow mapping
            for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
            {
                MATERIAL_APPLY_OR_FAIL(Shaders.SetArrayUniformByIndex(m_pbrLocations.lightSpaces, i, &m_directionalLightSpace[i]));
            }
            // Global shader options
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.usePCF, &m_usePCF));

            // HACK:
            f32 bias = 0.00005f;
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.bias, &bias));
        }
        else
        {
            ERROR_LOG("Unrecognized shader id: '{}'.", shaderId);
            return false;
        }

        MATERIAL_APPLY_OR_FAIL(Shaders.ApplyGlobal(frameData, true));

        // Sync the frameNumber and draw index
        s->frameNumber = frameData.frameNumber;
        s->drawIndex   = frameData.drawIndex;
        return true;
    }

    bool MaterialSystem::ApplyPointLights(Material* material, u16 pLightsLoc, u16 numPLightsLoc) const
    {
        const auto pointLights = Lights.GetPointLights();
        const auto numPLights  = pointLights.Size();

        MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(pLightsLoc, pointLights.GetData()));
        MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(numPLightsLoc, &numPLights));

        return true;
    }

    bool MaterialSystem::ApplyInstance(Material* material, const FrameData& frameData, const bool needsUpdate) const
    {
        MATERIAL_APPLY_OR_FAIL(Shaders.BindInstance(material->internalId))
        if (needsUpdate)
        {
            if (material->shaderId == m_pbrShaderId)
            {
                // PBR Shader
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.properties, material->properties));

                // Material maps
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetArrayUniformByIndex(m_pbrLocations.materialTextures, PBR_SAMP_ALBEDO, &material->maps[PBR_SAMP_ALBEDO]));
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetArrayUniformByIndex(m_pbrLocations.materialTextures, PBR_SAMP_NORMAL, &material->maps[PBR_SAMP_NORMAL]));
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetArrayUniformByIndex(m_pbrLocations.materialTextures, PBR_SAMP_COMBINED, &material->maps[PBR_SAMP_COMBINED]));

                // Shadow map
                material->maps[PBR_SAMP_SHADOW_MAP].texture = m_currentShadowTexture;
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.shadowTextures, &material->maps[PBR_SAMP_SHADOW_MAP]));

                // Irradiance map
                material->maps[PBR_SAMP_IRRADIANCE_MAP].texture =
                    material->irradianceTexture != INVALID_ID ? material->irradianceTexture : m_currentIrradianceTexture;

                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.iblCubeTexture, &material->maps[PBR_SAMP_IRRADIANCE_MAP]));

                // Directional light
                const auto dirLight = Lights.GetDirectionalLight();
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.dirLight, &dirLight->data));

                // Point lights
                if (!ApplyPointLights(material, m_pbrLocations.pLights, m_pbrLocations.numPLights))
                {
                    return false;
                }
            }
            else if (material->shaderId == m_terrainShaderId)
            {
                // Apply Material maps
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.materialTextures, &material->maps[TERRAIN_SAMP_MATERIALS]));

                // Apply Shadow map
                material->maps[TERRAIN_SAMP_SHADOW_MAP].texture = m_currentShadowTexture;
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.shadowTextures, &material->maps[TERRAIN_SAMP_SHADOW_MAP]));

                // Apply Irradiance map
                material->maps[TERRAIN_SAMP_IRRADIANCE_MAP].texture =
                    material->irradianceTexture != INVALID_ID ? material->irradianceTexture : m_currentIrradianceTexture;

                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.iblCubeTexture, &material->maps[TERRAIN_SAMP_IRRADIANCE_MAP]));

                // Apply properties
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.properties, material->properties));

                // Apply point lights
                if (!ApplyPointLights(material, m_terrainLocations.pLights, m_terrainLocations.numPLights))
                {
                    return false;
                }
            }
            else
            {
                ERROR_LOG("Unrecognized shader id: '{}' on material: '{}'.", material->shaderId, material->name);
                return false;
            }
        }

        MATERIAL_APPLY_OR_FAIL(Shaders.ApplyInstance(frameData, needsUpdate))
        return true;
    }

    bool MaterialSystem::ApplyLocal(const FrameData& frameData, Material* material, const mat4* model) const
    {
        Shaders.BindLocal();
        bool result = false;
        if (material->shaderId == m_pbrShaderId)
        {
            result = Shaders.SetUniformByIndex(m_pbrLocations.model, model);
        }
        if (material->shaderId == m_terrainShaderId)
        {
            result = Shaders.SetUniformByIndex(m_terrainLocations.model, model);
        }
        Shaders.ApplyLocal(frameData);

        if (!result)
        {
            ERROR_LOG("Unrecognized shader id: '{}' on material: '{}'.", material->shaderId, material->name);
        }
        return result;
    }

    Material* MaterialSystem::GetDefault()
    {
        if (!m_initialized)
        {
            FATAL_LOG("Tried to get the default Material before system is initialized.");
            return nullptr;
        }
        return &m_defaultPbrMaterial;
    }

    Material* MaterialSystem::GetDefaultTerrain()
    {
        if (!m_initialized)
        {
            FATAL_LOG("Tried to get the default Terrain Material before system is initialized.");
            return nullptr;
        }
        return &m_defaultTerrainMaterial;
    }

    Material* MaterialSystem::GetDefaultPbr()
    {
        if (!m_initialized)
        {
            FATAL_LOG("Tried to get the default PBR Material before system is initialized.");
            return nullptr;
        }
        return &m_defaultPbrMaterial;
    }

    bool MaterialSystem::CreateDefaultTerrainMaterial()
    {
        m_defaultTerrainMaterial.name           = DEFAULT_TERRAIN_MATERIAL_NAME;
        m_defaultTerrainMaterial.type           = MaterialType::Terrain;
        m_defaultTerrainMaterial.propertiesSize = sizeof(MaterialTerrainProperties);
        m_defaultTerrainMaterial.properties     = Memory.Allocate<MaterialTerrainProperties>(MemoryType::MaterialInstance);

        auto props                       = static_cast<MaterialTerrainProperties*>(m_defaultTerrainMaterial.properties);
        props->numMaterials              = TERRAIN_MAX_MATERIAL_COUNT;
        props->materials[0].diffuseColor = vec4(1);
        props->materials[0].shininess    = 8.0f;
        props->materials[0].padding      = vec3(0);

        m_defaultTerrainMaterial.maps.Resize(TERRAIN_TOTAL_MAP_COUNT);
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_MATERIALS].texture  = Textures.GetDefaultTerrain();
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP].texture = Textures.GetDefaultDiffuse();
        // Ensure that the shadow texture map has repeat mode of ClampToBorder
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP].repeatU     = TextureRepeat::ClampToBorder;
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP].repeatV     = TextureRepeat::ClampToBorder;
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP].repeatW     = TextureRepeat::ClampToBorder;
        m_defaultTerrainMaterial.maps[TERRAIN_SAMP_IRRADIANCE_MAP].texture = Textures.GetDefaultCube();

        auto& mat = m_defaultTerrainMaterial;

        // Acquire instance resource for all our maps
        ShaderInstanceResourceConfig instanceResourceConfig;
        // 3 configs: material maps, shadows maps and irradiance map
        instanceResourceConfig.uniformConfigCount = 3;
        instanceResourceConfig.uniformConfigs     = Memory.Allocate<ShaderInstanceUniformTextureConfig>(MemoryType::Array, 3);

        // Material textures
        auto& matTextures           = instanceResourceConfig.uniformConfigs[0];
        matTextures.uniformLocation = m_terrainLocations.materialTextures;
        matTextures.textureMapCount = 1;
        matTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, matTextures.textureMapCount);
        matTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_MATERIALS];

        // Shadow textures
        auto& shadowTextures           = instanceResourceConfig.uniformConfigs[1];
        shadowTextures.uniformLocation = m_terrainLocations.shadowTextures;
        shadowTextures.textureMapCount = 1;
        shadowTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, shadowTextures.textureMapCount);
        shadowTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_SHADOW_MAP];

        // Irradiance cube texture
        auto& iblTextures           = instanceResourceConfig.uniformConfigs[2];
        iblTextures.uniformLocation = m_terrainLocations.iblCubeTexture;
        iblTextures.textureMapCount = 1;
        iblTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, iblTextures.textureMapCount);
        iblTextures.textureMaps[0]  = &mat.maps[TERRAIN_SAMP_IRRADIANCE_MAP];

        Shader* shader = Shaders.GetById(m_terrainShaderId);
        if (!Renderer.AcquireShaderInstanceResources(*shader, instanceResourceConfig, mat.internalId))
        {
            ERROR_LOG("Failed to acquire renderer resources for material: '{}'.", mat.name);
        }

        // Free our uniform config texture maps
        for (u32 i = 0; i < instanceResourceConfig.uniformConfigCount; i++)
        {
            auto& cfg = instanceResourceConfig.uniformConfigs[i];
            Memory.Free(cfg.textureMaps);
            cfg.textureMaps = nullptr;
        }

        // Free our uniform configs array
        Memory.Free(instanceResourceConfig.uniformConfigs);

        m_defaultTerrainMaterial.shaderId = shader->id;

        return true;
    }

    bool MaterialSystem::CreateDefaultPbrMaterial()
    {
        m_defaultPbrMaterial.name           = DEFAULT_PBR_MATERIAL_NAME;
        m_defaultPbrMaterial.type           = MaterialType::PBR;
        m_defaultPbrMaterial.propertiesSize = sizeof(MaterialPhongProperties);
        m_defaultPbrMaterial.properties     = Memory.Allocate<MaterialPhongProperties>(MemoryType::MaterialInstance);

        auto props          = static_cast<MaterialPhongProperties*>(m_defaultPbrMaterial.properties);
        props->diffuseColor = vec4(1);
        props->shininess    = 8.0f;

        m_defaultPbrMaterial.maps.Resize(PBR_TOTAL_MAP_COUNT);
        m_defaultPbrMaterial.maps[PBR_SAMP_ALBEDO].texture = Textures.GetDefault();
        // Set minify and magnify to nearest to reduce bluriness on checkerboard default texture
        m_defaultPbrMaterial.maps[PBR_SAMP_ALBEDO].minifyFilter  = TextureFilter::ModeNearest;
        m_defaultPbrMaterial.maps[PBR_SAMP_ALBEDO].magnifyFilter = TextureFilter::ModeNearest;
        m_defaultPbrMaterial.maps[PBR_SAMP_NORMAL].texture       = Textures.GetDefaultNormal();
        m_defaultPbrMaterial.maps[PBR_SAMP_COMBINED].texture     = Textures.GetDefaultCombined();
        m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP].texture   = Textures.GetDefaultAlbedo();
        // Change the clamp mode for the default shadow maps to border
        m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP].repeatU = TextureRepeat::ClampToBorder;
        m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP].repeatV = TextureRepeat::ClampToBorder;
        m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP].repeatW = TextureRepeat::ClampToBorder;

        m_defaultPbrMaterial.maps[PBR_SAMP_IRRADIANCE_MAP].texture = Textures.GetDefaultCube();

        auto& mat = m_defaultPbrMaterial;

        ShaderInstanceResourceConfig instanceConfig;
        instanceConfig.uniformConfigCount = 3;
        instanceConfig.uniformConfigs =
            Memory.Allocate<ShaderInstanceUniformTextureConfig>(MemoryType::Array, instanceConfig.uniformConfigCount);

        // Material textures
        auto& matTextures                          = instanceConfig.uniformConfigs[0];
        matTextures.uniformLocation                = m_pbrLocations.materialTextures;
        matTextures.textureMapCount                = PBR_MATERIAL_TEXTURE_COUNT;
        matTextures.textureMaps                    = Memory.Allocate<TextureMap*>(MemoryType::Array, matTextures.textureMapCount);
        matTextures.textureMaps[PBR_SAMP_ALBEDO]   = &mat.maps[PBR_SAMP_ALBEDO];
        matTextures.textureMaps[PBR_SAMP_NORMAL]   = &mat.maps[PBR_SAMP_NORMAL];
        matTextures.textureMaps[PBR_SAMP_COMBINED] = &mat.maps[PBR_SAMP_COMBINED];

        // Shadow textures
        auto& shadowTextures           = instanceConfig.uniformConfigs[1];
        shadowTextures.uniformLocation = m_pbrLocations.shadowTextures;
        shadowTextures.textureMapCount = 1;
        shadowTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, shadowTextures.textureMapCount);
        shadowTextures.textureMaps[0]  = &mat.maps[PBR_SAMP_SHADOW_MAP];

        // Irradiance cube texture
        auto& irradianceTextures           = instanceConfig.uniformConfigs[2];
        irradianceTextures.uniformLocation = m_pbrLocations.shadowTextures;
        irradianceTextures.textureMapCount = 1;
        irradianceTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, irradianceTextures.textureMapCount);
        irradianceTextures.textureMaps[0]  = &mat.maps[PBR_SAMP_IRRADIANCE_MAP];

        Shader* shader = Shaders.GetById(m_pbrShaderId);
        if (!Renderer.AcquireShaderInstanceResources(*shader, instanceConfig, mat.internalId))
        {
            ERROR_LOG("Failed to acquire renderer resources for the Default PBR Material.");
            return false;
        }

        // Free our uniform config texture maps
        for (u32 i = 0; i < instanceConfig.uniformConfigCount; i++)
        {
            auto& cfg = instanceConfig.uniformConfigs[i];
            Memory.Free(cfg.textureMaps);
            cfg.textureMaps = nullptr;
        }

        // Free our uniform configs array
        Memory.Free(instanceConfig.uniformConfigs);

        // Assign the shader id to the default material
        m_defaultPbrMaterial.shaderId = shader->id;
        return true;
    }

    bool MaterialSystem::AssignMap(TextureMap& map, const MaterialConfigMap& config, TextureHandle defaultTexture) const
    {
        map = TextureMap(config);
        if (!config.textureName.Empty())
        {
            map.texture = Textures.Acquire(config.textureName.Data(), true);
            if (!map.texture)
            {
                WARN_LOG("Unable to load texture: '{}' for material: '{}', using the default instead.", config.textureName, config.name);
                map.texture = defaultTexture;
            }
        }
        else
        {
            map.texture = defaultTexture;
        }

        if (!Renderer.AcquireTextureMapResources(map))
        {
            ERROR_LOG("Failed to acquire resource for texture map for material: '{}'.", config.name);
            return false;
        }

        return true;
    }

    bool MaterialSystem::LoadMaterial(const MaterialConfig& config, Material& mat) const
    {
        // Name
        mat.name = config.name;
        // Id of the shader associated with this material
        mat.shaderId = Shaders.GetId(config.shaderName);
        // Copy over the type of the material
        mat.type = config.type;

        u32 mapCount = 0;

        if (config.type == MaterialType::PBR)
        {
            mat.propertiesSize = sizeof(MaterialPhongProperties);
            mat.properties     = Memory.Allocate<MaterialPhongProperties>(MemoryType::MaterialInstance);
            // Defaults
            auto props          = static_cast<MaterialPhongProperties*>(mat.properties);
            props->diffuseColor = vec4(1);
            props->shininess    = 32.0f;
            props->padding      = vec3(0);

            for (const auto& prop : config.props)
            {
                if (prop.name.IEquals("diffuseColor"))
                {
                    props->diffuseColor = std::get<vec4>(prop.value);
                }
                else if (prop.name.IEquals("shininess"))
                {
                    props->shininess = std::get<f32>(prop.value);
                }
            }

            // For PBR materials we expect 3 material maps (albedo, normal and combined(metallic, roughness and ao)) + shadow and irradiance
            mat.maps.Resize(PBR_TOTAL_MAP_COUNT);

            bool albedoAssigned   = false;
            bool normalAssigned   = false;
            bool combinedAssigned = false;
            bool cubeAssigned     = false;

            for (const auto& map : config.maps)
            {
                if (map.name.IEquals("albedo"))
                {
                    if (!AssignMap(mat.maps[PBR_SAMP_ALBEDO], map, Textures.GetDefaultAlbedo()))
                    {
                        return false;
                    }
                    albedoAssigned = true;
                }
                else if (map.name.IEquals("normal"))
                {
                    if (!AssignMap(mat.maps[PBR_SAMP_NORMAL], map, Textures.GetDefaultNormal()))
                    {
                        return false;
                    }
                    normalAssigned = true;
                }
                else if (map.name.IEquals("combined"))
                {
                    if (!AssignMap(mat.maps[PBR_SAMP_COMBINED], map, Textures.GetDefaultCombined()))
                    {
                        return false;
                    }
                    combinedAssigned = true;
                }
                else if (map.name.IEquals("iblCube"))
                {
                    if (!AssignMap(mat.maps[PBR_SAMP_IRRADIANCE_MAP], map, Textures.GetDefaultCube()))
                    {
                        return false;
                    }
                    cubeAssigned = true;
                }
            }

            // Shadow map
            {
                MaterialConfigMap mapConfig;
                mapConfig.minifyFilter  = TextureFilter::ModeLinear;
                mapConfig.magnifyFilter = TextureFilter::ModeLinear;
                mapConfig.repeatU       = TextureRepeat::ClampToBorder;
                mapConfig.repeatV       = TextureRepeat::ClampToBorder;
                mapConfig.repeatW       = TextureRepeat::ClampToBorder;
                mapConfig.name          = "shadowMap";
                mapConfig.textureName   = "";
                if (!AssignMap(mat.maps[PBR_SAMP_SHADOW_MAP], mapConfig, Textures.GetDefaultAlbedo()))
                {
                    return false;
                }
            }

            if (!albedoAssigned)
            {
                MaterialConfigMap map("albedo", "");
                if (!AssignMap(mat.maps[PBR_SAMP_ALBEDO], map, Textures.GetDefaultAlbedo()))
                {
                    return false;
                }
            }

            if (!normalAssigned)
            {
                MaterialConfigMap map("normal", "");
                if (!AssignMap(mat.maps[PBR_SAMP_NORMAL], map, Textures.GetDefaultNormal()))
                {
                    return false;
                }
            }

            if (!combinedAssigned)
            {
                MaterialConfigMap map("combined", "");
                if (!AssignMap(mat.maps[PBR_SAMP_COMBINED], map, Textures.GetDefaultCombined()))
                {
                    return false;
                }
            }

            if (!cubeAssigned)
            {
                MaterialConfigMap map("iblCube", "");
                if (!AssignMap(mat.maps[PBR_SAMP_IRRADIANCE_MAP], map, Textures.GetDefaultCube()))
                {
                    return false;
                }
            }
        }
        else if (config.type == MaterialType::Custom)
        {
            // Calculate the needed space for the property struct
            mat.propertiesSize = 0;
            for (const auto& prop : config.props)
            {
                mat.propertiesSize += prop.size;
            }
            // Allocate enough space to hold all the structure for all the properties
            mat.properties = Memory.AllocateBlock(MemoryType::MaterialInstance, mat.propertiesSize);

            u32 offset = 0;
            for (const auto& prop : config.props)
            {
                if (prop.size > 0)
                {
                    std::visit(
                        [&](auto& v) {
                            std::memcpy(static_cast<char*>(mat.properties) + offset, &v, prop.size);
                            offset += prop.size;
                        },
                        prop.value);
                }
            }

            mat.maps.Resize(config.maps.Size());
            mapCount = config.maps.Size();

            for (u32 i = 0; i < config.maps.Size(); i++)
            {
                // No known mapping so we just copy over the maps in the order they are provided in the config
                // We know nothing about the maps so we assume just a default texture when we find an invalid one
                if (!AssignMap(mat.maps[i], config.maps[i], Textures.GetDefault()))
                {
                    return false;
                }
            }
        }
        else
        {
            ERROR_LOG("Unsupported MaterialType: '{}'.", ToString(config.type));
            return false;
        }

        ShaderInstanceResourceConfig instanceConfig;

        Shader* shader = nullptr;
        switch (config.type)
        {
            case MaterialType::PBR:
            {
                shader = Shaders.Get(config.shaderName.Empty() ? "Shader.PBR" : config.shaderName);

                instanceConfig.uniformConfigCount = 3;
                instanceConfig.uniformConfigs =
                    Memory.Allocate<ShaderInstanceUniformTextureConfig>(MemoryType::Array, instanceConfig.uniformConfigCount);

                // Material textures
                auto& matTextures                          = instanceConfig.uniformConfigs[0];
                matTextures.uniformLocation                = m_pbrLocations.materialTextures;
                matTextures.textureMapCount                = PBR_MATERIAL_TEXTURE_COUNT;
                matTextures.textureMaps                    = Memory.Allocate<TextureMap*>(MemoryType::Array, matTextures.textureMapCount);
                matTextures.textureMaps[PBR_SAMP_ALBEDO]   = &mat.maps[PBR_SAMP_ALBEDO];
                matTextures.textureMaps[PBR_SAMP_NORMAL]   = &mat.maps[PBR_SAMP_NORMAL];
                matTextures.textureMaps[PBR_SAMP_COMBINED] = &mat.maps[PBR_SAMP_COMBINED];

                // Shadow textures
                auto& shadowTextures           = instanceConfig.uniformConfigs[1];
                shadowTextures.uniformLocation = m_pbrLocations.shadowTextures;
                shadowTextures.textureMapCount = 1;
                shadowTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, shadowTextures.textureMapCount);
                shadowTextures.textureMaps[0]  = &mat.maps[PBR_SAMP_SHADOW_MAP];

                // Irradiance cube texture
                auto& irradianceTextures           = instanceConfig.uniformConfigs[2];
                irradianceTextures.uniformLocation = m_pbrLocations.shadowTextures;
                irradianceTextures.textureMapCount = 1;
                irradianceTextures.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, irradianceTextures.textureMapCount);
                irradianceTextures.textureMaps[0]  = &mat.maps[PBR_SAMP_IRRADIANCE_MAP];
            }
            break;
            case MaterialType::Custom:
            {
                if (config.shaderName.Empty())
                {
                    FATAL_LOG("Custom Material: '{}' does not have a Shader name which is required.", config.name);
                    return false;
                }
                shader = Shaders.Get(config.shaderName);

                u32 globalSamplerCount   = shader->globalUniformSamplerCount;
                u32 instanceSamplerCount = shader->instanceUniformSamplerCount;

                instanceConfig.uniformConfigCount = globalSamplerCount + instanceSamplerCount;
                instanceConfig.uniformConfigs =
                    Memory.Allocate<ShaderInstanceUniformTextureConfig>(MemoryType::Array, instanceConfig.uniformConfigCount);

                // Tack the number of maps used by global uniforms first and offset by that
                u32 mapOffset = globalSamplerCount;

                for (u32 i = 0; i < instanceSamplerCount; ++i)
                {
                    auto& u                       = shader->uniforms[shader->instanceSamplers[i]];
                    auto& uniformConfig           = instanceConfig.uniformConfigs[i];
                    uniformConfig.uniformLocation = u.location;
                    uniformConfig.textureMapCount = Max(u.arrayLength, (u8)1);
                    uniformConfig.textureMaps     = Memory.Allocate<TextureMap*>(MemoryType::Array, uniformConfig.textureMapCount);
                    for (u32 j = 0; j < uniformConfig.textureMapCount; ++j)
                    {
                        uniformConfig.textureMaps[j] = &mat.maps[i + mapOffset];
                    }
                }
            }
            break;
            default:
                ERROR_LOG("Unsupported Material type: '{}'.", ToString(config.type));
                return false;
        }

        auto result = Renderer.AcquireShaderInstanceResources(*shader, instanceConfig, mat.internalId);
        if (!result)
        {
            ERROR_LOG("Failed to acquire renderer resources for Material: '{}'.", mat.name);
        }

        // Free our uniform config texture maps
        for (u32 i = 0; i < instanceConfig.uniformConfigCount; i++)
        {
            auto& cfg = instanceConfig.uniformConfigs[i];
            Memory.Free(cfg.textureMaps);
            cfg.textureMaps = nullptr;
        }

        // Free our uniform configs array
        Memory.Free(instanceConfig.uniformConfigs);

        return result;
    }

    void MaterialSystem::DestroyMaterial(Material& mat) const
    {
        INFO_LOG("Destroying: '{}'.", mat.name);

        // Release all associated maps
        for (auto& map : mat.maps)
        {
            if (map.texture)
            {
                // Release the associated texture reference
                Textures.Release(map.texture);
            }

            // Release texture map resources
            Renderer.ReleaseTextureMapResources(map);
        }

        // Release renderer resources
        if (mat.shaderId != INVALID_ID && mat.internalId != INVALID_ID)
        {
            Shader* shader = Shaders.GetById(mat.shaderId);
            Renderer.ReleaseShaderInstanceResources(*shader, mat.internalId);
            mat.shaderId = INVALID_ID;
        }

        // Release all associated properties
        if (mat.properties && mat.propertiesSize > 0)
        {
            Memory.Free(mat.properties);
        }

        // Zero out memory and invalidate ids
        mat.id                = INVALID_ID;
        mat.generation        = INVALID_ID;
        mat.internalId        = INVALID_ID;
        mat.renderFrameNumber = INVALID_ID;
        mat.name.Destroy();
    }
}  // namespace C3D
