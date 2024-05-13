
#include "material_system.h"

#include "core/clock.h"
#include "core/engine.h"
#include "core/frame_data.h"
#include "core/logger.h"
#include "core/string_utils.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_types.h"
#include "resources/loaders/material_loader.h"
#include "resources/shaders/shader.h"
#include "systems/cvars/cvar_system.h"
#include "systems/lights/light_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "MATERIAL_SYSTEM";

    MaterialSystem::MaterialSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool MaterialSystem::OnInit(const MaterialSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        if (config.maxMaterialCount == 0)
        {
            ERROR_LOG("config.maxTextureCount must be > 0.");
            return false;
        }

        m_config = config;

        m_materialShaderId = INVALID_ID;

        // Create our hashmap for the materials
        m_registeredMaterials.Create(config.maxMaterialCount);

        if (!CreateDefaultMaterial())
        {
            ERROR_LOG("Failed to create Default Material.");
            return false;
        }

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

        // Get the uniform indices and save them off for quick lookups
        // Start with the Material shader
        Shader* shader                      = Shaders.Get("Shader.Builtin.Material");
        m_materialShaderId                  = shader->id;
        m_materialLocations.projection      = Shaders.GetUniformIndex(shader, "projection");
        m_materialLocations.view            = Shaders.GetUniformIndex(shader, "view");
        m_materialLocations.ambientColor    = Shaders.GetUniformIndex(shader, "ambientColor");
        m_materialLocations.properties      = Shaders.GetUniformIndex(shader, "properties");
        m_materialLocations.viewPosition    = Shaders.GetUniformIndex(shader, "viewPosition");
        m_materialLocations.diffuseTexture  = Shaders.GetUniformIndex(shader, "diffuseTexture");
        m_materialLocations.specularTexture = Shaders.GetUniformIndex(shader, "specularTexture");
        m_materialLocations.normalTexture   = Shaders.GetUniformIndex(shader, "normalTexture");
        m_materialLocations.model           = Shaders.GetUniformIndex(shader, "model");
        m_materialLocations.renderMode      = Shaders.GetUniformIndex(shader, "mode");
        m_materialLocations.dirLight        = Shaders.GetUniformIndex(shader, "dirLight");
        m_materialLocations.pLights         = Shaders.GetUniformIndex(shader, "pLights");
        m_materialLocations.numPLights      = Shaders.GetUniformIndex(shader, "numPLights");

        // Then get the PBR Shader
        shader                          = Shaders.Get("Shader.PBR");
        m_pbrShaderId                   = shader->id;
        m_pbrLocations.projection       = Shaders.GetUniformIndex(shader, "projection");
        m_pbrLocations.view             = Shaders.GetUniformIndex(shader, "view");
        m_pbrLocations.lightSpace_0     = Shaders.GetUniformIndex(shader, "lightSpace_0");
        m_pbrLocations.lightSpace_1     = Shaders.GetUniformIndex(shader, "lightSpace_1");
        m_pbrLocations.lightSpace_2     = Shaders.GetUniformIndex(shader, "lightSpace_2");
        m_pbrLocations.lightSpace_3     = Shaders.GetUniformIndex(shader, "lightSpace_3");
        m_pbrLocations.cascadeSplits    = Shaders.GetUniformIndex(shader, "cascadeSplits");
        m_pbrLocations.viewPosition     = Shaders.GetUniformIndex(shader, "viewPosition");
        m_pbrLocations.properties       = Shaders.GetUniformIndex(shader, "properties");
        m_pbrLocations.iblCubeTexture   = Shaders.GetUniformIndex(shader, "iblCubeTexture");
        m_pbrLocations.albedoTexture    = Shaders.GetUniformIndex(shader, "albedoTexture");
        m_pbrLocations.normalTexture    = Shaders.GetUniformIndex(shader, "normalTexture");
        m_pbrLocations.metallicTexture  = Shaders.GetUniformIndex(shader, "metallicTexture");
        m_pbrLocations.roughnessTexture = Shaders.GetUniformIndex(shader, "roughnessTexture");
        m_pbrLocations.aoTexture        = Shaders.GetUniformIndex(shader, "aoTexture");
        m_pbrLocations.shadowTexture_0  = Shaders.GetUniformIndex(shader, "shadowTexture_0");
        m_pbrLocations.shadowTexture_1  = Shaders.GetUniformIndex(shader, "shadowTexture_1");
        m_pbrLocations.shadowTexture_2  = Shaders.GetUniformIndex(shader, "shadowTexture_2");
        m_pbrLocations.shadowTexture_3  = Shaders.GetUniformIndex(shader, "shadowTexture_3");
        m_pbrLocations.model            = Shaders.GetUniformIndex(shader, "model");
        m_pbrLocations.renderMode       = Shaders.GetUniformIndex(shader, "mode");
        m_pbrLocations.dirLight         = Shaders.GetUniformIndex(shader, "dirLight");
        m_pbrLocations.pLights          = Shaders.GetUniformIndex(shader, "pLights");
        m_pbrLocations.numPLights       = Shaders.GetUniformIndex(shader, "numPLights");
        m_pbrLocations.usePCF           = Shaders.GetUniformIndex(shader, "usePCF");
        m_pbrLocations.bias             = Shaders.GetUniformIndex(shader, "bias");

        // Finally the Terrain Shader
        shader                             = Shaders.Get("Shader.Builtin.Terrain");
        m_terrainShaderId                  = shader->id;
        m_terrainLocations.projection      = Shaders.GetUniformIndex(shader, "projection");
        m_terrainLocations.view            = Shaders.GetUniformIndex(shader, "view");
        m_terrainLocations.lightSpace_0    = Shaders.GetUniformIndex(shader, "lightSpace_0");
        m_terrainLocations.lightSpace_1    = Shaders.GetUniformIndex(shader, "lightSpace_1");
        m_terrainLocations.lightSpace_2    = Shaders.GetUniformIndex(shader, "lightSpace_2");
        m_terrainLocations.lightSpace_3    = Shaders.GetUniformIndex(shader, "lightSpace_3");
        m_terrainLocations.cascadeSplits   = Shaders.GetUniformIndex(shader, "cascadeSplits");
        m_terrainLocations.viewPosition    = Shaders.GetUniformIndex(shader, "viewPosition");
        m_terrainLocations.model           = Shaders.GetUniformIndex(shader, "model");
        m_terrainLocations.renderMode      = Shaders.GetUniformIndex(shader, "mode");
        m_terrainLocations.dirLight        = Shaders.GetUniformIndex(shader, "dirLight");
        m_terrainLocations.pLights         = Shaders.GetUniformIndex(shader, "pLights");
        m_terrainLocations.numPLights      = Shaders.GetUniformIndex(shader, "numPLights");
        m_terrainLocations.properties      = Shaders.GetUniformIndex(shader, "properties");
        m_terrainLocations.iblCubeTexture  = Shaders.GetUniformIndex(shader, "iblCubeTexture");
        m_terrainLocations.shadowTexture_0 = Shaders.GetUniformIndex(shader, "shadowTexture_0");
        m_terrainLocations.shadowTexture_1 = Shaders.GetUniformIndex(shader, "shadowTexture_1");
        m_terrainLocations.shadowTexture_2 = Shaders.GetUniformIndex(shader, "shadowTexture_2");
        m_terrainLocations.shadowTexture_3 = Shaders.GetUniformIndex(shader, "shadowTexture_3");
        m_terrainLocations.usePCF          = Shaders.GetUniformIndex(shader, "usePCF");
        m_terrainLocations.bias            = Shaders.GetUniformIndex(shader, "bias");

        for (u32 i = 0; i < TERRAIN_MAX_MATERIAL_COUNT; i++)
        {
            auto num           = String(i);
            auto albedoName    = "albedoTexture_" + num;
            auto normalName    = "normalTexture_" + num;
            auto metallicName  = "metallicTexture_" + num;
            auto roughnessName = "roughnessTexture_" + num;
            auto aoName        = "aoTexture_" + num;

            m_terrainLocations.samplers[i * TERRAIN_PER_MATERIAL_SAMP_COUNT + 0] = Shaders.GetUniformIndex(shader, albedoName.Data());
            m_terrainLocations.samplers[i * TERRAIN_PER_MATERIAL_SAMP_COUNT + 1] = Shaders.GetUniformIndex(shader, normalName.Data());
            m_terrainLocations.samplers[i * TERRAIN_PER_MATERIAL_SAMP_COUNT + 2] = Shaders.GetUniformIndex(shader, metallicName.Data());
            m_terrainLocations.samplers[i * TERRAIN_PER_MATERIAL_SAMP_COUNT + 3] = Shaders.GetUniformIndex(shader, roughnessName.Data());
            m_terrainLocations.samplers[i * TERRAIN_PER_MATERIAL_SAMP_COUNT + 4] = Shaders.GetUniformIndex(shader, aoName.Data());
        }

        // Set our current irradiance texture to the default
        m_currentIrradianceTexture = Textures.GetDefaultCube();
        // Set all shadow textures to default diffuse
        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
        {
            m_currentShadowTexture[i] = Textures.GetDefaultDiffuse();
        }

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
        for (auto& ref : m_registeredMaterials)
        {
            if (ref.material.id != INVALID_ID)
            {
                DestroyMaterial(ref.material);
            }
        }

        INFO_LOG("Destroying default materials.");
        DestroyMaterial(m_defaultMaterial);
        DestroyMaterial(m_defaultTerrainMaterial);
        DestroyMaterial(m_defaultPbrMaterial);

        // Cleanup our registered Material hashmap
        m_registeredMaterials.Destroy();
    }

    Material* MaterialSystem::Acquire(const String& name)
    {
        if (m_registeredMaterials.Has(name))
        {
            // The material already exists
            MaterialReference& ref = m_registeredMaterials.Get(name);
            ref.referenceCount++;

            TRACE("Material: '{}' already exists. The refCount is now: {}.", name, ref.referenceCount);

            return &ref.material;
        }

        MaterialConfig materialConfig;
        if (!Resources.Load(name, materialConfig))
        {
            ERROR_LOG("Failed to load material resource: '{}'. Returning nullptr.", name);
            return nullptr;
        }

        Material* m = AcquireFromConfig(materialConfig);
        Resources.Unload(materialConfig);

        if (!m)
        {
            ERROR_LOG("Failed to acquire material from config: '{}'. Returning nullptr.", name);
        }
        return m;
    }

    Material& MaterialSystem::AcquireReference(const String& name, bool autoRelease, bool& needsCreation)
    {
        if (m_registeredMaterials.Has(name))
        {
            // The material already exists
            MaterialReference& ref = m_registeredMaterials.Get(name);
            ref.referenceCount++;

            TRACE("Material: '{}' already exists. The refCount is now: {}.", name, ref.referenceCount);

            needsCreation = false;
            return ref.material;
        }

        // The material does not exist yet
        // Add a new reference into the registered materials hashmap
        m_registeredMaterials.Set(name, MaterialReference(autoRelease));

        // Get a reference to it so we can start using it
        auto& ref = m_registeredMaterials.Get(name);
        // Set the material id to the index into the registered material hashmap
        ref.material.id = m_registeredMaterials.GetIndex(name);
        // Mark that this material still needs to be created
        needsCreation = true;
        // Return a pointer to the material
        return ref.material;
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
            const auto materialCount = materialNames.Size();

            // Acquire all the internal materials by name
            DynamicArray<Material*> materials(materialCount);
            for (const auto& name : materialNames)
            {
                materials.PushBack(Acquire(name));
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
            props->numMaterials = materialCount;
            props->padding2     = vec4(0);

            // 5 Maps per material + one for the irradiance map and one for the shadow map. Allocate enough space for all materials
            mat.maps.Resize(TERRAIN_SAMP_COUNT_TOTAL);

            // Map names and default fallback textures
            const char* mapNames[TERRAIN_PER_MATERIAL_SAMP_COUNT]     = { "diffuse", "normal", "metallic", "roughness", "ao" };
            Texture* defaultTextures[TERRAIN_PER_MATERIAL_SAMP_COUNT] = { Textures.GetDefaultDiffuse(), Textures.GetDefaultNormal(),
                                                                          Textures.GetDefaultMetallic(), Textures.GetDefaultRoughness(),
                                                                          Textures.GetDefaultAo() };

            // Use the Default Material for unassigned slots
            Material* defaultMaterial = GetDefaultPbr();

            // Phong properties and maps for each material
            for (u32 materialIndex = 0; materialIndex < TERRAIN_MAX_MATERIAL_COUNT; materialIndex++)
            {
                // Properties
                auto& matProps = props->materials[materialIndex];
                // Use default material unless within the materialCount
                Material* refMat = defaultMaterial;
                if (materialIndex < materialCount)
                {
                    refMat = materials[materialIndex];
                }

                auto p                = static_cast<MaterialPhongProperties*>(refMat->properties);
                matProps.diffuseColor = p->diffuseColor;
                matProps.padding      = vec3(0);
                matProps.shininess    = p->shininess;

                // For PBR we have 5 maps (Albedo, Normal, Metallic, Roughness and AO)
                for (u32 mapIndex = 0; mapIndex < TERRAIN_PER_MATERIAL_SAMP_COUNT; mapIndex++)
                {
                    MaterialConfigMap mapConfig;
                    mapConfig.name          = mapNames[mapIndex];
                    mapConfig.repeatU       = refMat->maps[mapIndex].repeatU;
                    mapConfig.repeatV       = refMat->maps[mapIndex].repeatV;
                    mapConfig.repeatW       = refMat->maps[mapIndex].repeatW;
                    mapConfig.minifyFilter  = refMat->maps[mapIndex].minifyFilter;
                    mapConfig.magnifyFilter = refMat->maps[mapIndex].magnifyFilter;
                    mapConfig.textureName   = refMat->maps[mapIndex].texture->name;
                    if (!AssignMap(mat.maps[(materialIndex * TERRAIN_PER_MATERIAL_SAMP_COUNT) + mapIndex], mapConfig,
                                   defaultTextures[mapIndex]))
                    {
                        ERROR_LOG("Failed to assign: '{}' texture map for terrain material index: {}.", mapNames[mapIndex], materialIndex);
                        return nullptr;
                    }
                }
            }

            // Shadow map
            {
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
                {
                    MaterialConfigMap mapConfig;
                    mapConfig.name          = "shadowMap";
                    mapConfig.repeatU       = TextureRepeat::ClampToBorder;
                    mapConfig.repeatV       = TextureRepeat::ClampToBorder;
                    mapConfig.repeatW       = TextureRepeat::ClampToBorder;
                    mapConfig.minifyFilter  = TextureFilter::ModeLinear;
                    mapConfig.magnifyFilter = TextureFilter::ModeLinear;
                    mapConfig.textureName   = "";
                    if (!AssignMap(mat.maps[TERRAIN_SAMP_SHADOW_MAP + i], mapConfig, Textures.GetDefaultDiffuse()))
                    {
                        ERROR_LOG("Failed to assign: {} texture map for terrain shadow map.", mapConfig.name);
                        return nullptr;
                    }
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

            // Release the reference materials
            for (const auto& name : materialNames)
            {
                Release(name);
            }

            materials.Clear();

            // Acquire instance resource for all our maps
            const TextureMap** maps = Memory.Allocate<const TextureMap*>(MemoryType::Array, TERRAIN_PER_MATERIAL_SAMP_COUNT);
            // Assign our material's maps
            for (u32 i = 0; i < TERRAIN_PER_MATERIAL_SAMP_COUNT; i++)
            {
                maps[i] = &mat.maps[i];
            }

            bool result = Renderer.AcquireShaderInstanceResources(*shader, TERRAIN_PER_MATERIAL_SAMP_COUNT, maps, &mat.internalId);
            if (!result)
            {
                ERROR_LOG("Failed to acquire renderer resources for material: '{}'.", mat.name);
            }

            if (maps)
            {
                Memory.Free(maps);
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

    Material* MaterialSystem::AcquireFromConfig(const MaterialConfig& config)
    {
        // Return Default Material
        if (config.name.IEquals(DEFAULT_MATERIAL_NAME))
        {
            return &m_defaultMaterial;
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
        if (name.IEquals(DEFAULT_MATERIAL_NAME) || name.IEquals(DEFAULT_UI_MATERIAL_NAME) || name.IEquals(DEFAULT_TERRAIN_MATERIAL_NAME))
        {
            WARN_LOG("Tried to release Default Material. This happens automatically on shutdown.");
            return;
        }

        if (!m_registeredMaterials.Has(name))
        {
            WARN_LOG("Tried to release a material that does not exist: '{}'.", name);
            return;
        }

        MaterialReference& ref = m_registeredMaterials.Get(name);
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
            m_registeredMaterials.Delete(nameCopy);

            TRACE("The Material: '{}' was released. The texture was unloaded because refCount = 0 and autoRelease = true.", nameCopy);
        }
        else
        {
            TRACE("The Material: '{}' now has a refCount = {} (autoRelease = {}).", name, ref.referenceCount, ref.autoRelease);
        }
    }

    bool MaterialSystem::SetIrradiance(Texture* irradianceCubeTexture)
    {
        if (!irradianceCubeTexture)
        {
            ERROR_LOG("Invalid irradiance cube texture provided.");
            return false;
        }

        if (irradianceCubeTexture->type != TextureTypeCube)
        {
            ERROR_LOG("Provied texture is not of type: TextureTypeCube.");
            return false;
        }

        m_currentIrradianceTexture = irradianceCubeTexture;

        return true;
    }

    void MaterialSystem::ResetIrradiance() { m_currentIrradianceTexture = Textures.GetDefaultCube(); }

    bool MaterialSystem::SetShadowMap(Texture* shadowTexture, u8 cascadeIndex)
    {
        if (shadowTexture)
        {
            m_currentShadowTexture[cascadeIndex] = shadowTexture;
        }
        return true;
    }

    void MaterialSystem::SetDirectionalLightSpaceMatrix(const mat4& lightSpace, u8 cascadeIndex)
    {
        m_directionalLightSpace[cascadeIndex] = lightSpace;
    }

#define MATERIAL_APPLY_OR_FAIL(expr)             \
    if (!(expr))                                 \
    {                                            \
        ERROR_LOG("Failed to apply: {}.", expr); \
        return false;                            \
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

        if (shaderId == m_materialShaderId)
        {
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.projection, projection));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.view, view));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.viewPosition, viewPosition));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.renderMode, &renderMode));
        }
        else if (shaderId == m_terrainShaderId)
        {
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.projection, projection));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.view, view));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.cascadeSplits, cascadeSplits));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.viewPosition, viewPosition));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.renderMode, &renderMode));
            // Light space for shadow mapping
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.lightSpace_0, &m_directionalLightSpace[0]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.lightSpace_1, &m_directionalLightSpace[1]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.lightSpace_2, &m_directionalLightSpace[2]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.lightSpace_3, &m_directionalLightSpace[3]));

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
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.cascadeSplits, cascadeSplits))
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.viewPosition, viewPosition));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.renderMode, &renderMode));
            // Light space for shadow mapping
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.lightSpace_0, &m_directionalLightSpace[0]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.lightSpace_1, &m_directionalLightSpace[1]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.lightSpace_2, &m_directionalLightSpace[2]));
            MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.lightSpace_3, &m_directionalLightSpace[3]));
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

        MATERIAL_APPLY_OR_FAIL(Shaders.ApplyGlobal(true));

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
            if (material->shaderId == m_materialShaderId)
            {
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.properties, material->properties));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.diffuseTexture, &material->maps[0]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.specularTexture, &material->maps[1]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.normalTexture, &material->maps[2]));

                const auto dirLight = Lights.GetDirectionalLight();
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_materialLocations.dirLight, &dirLight->data));

                if (!ApplyPointLights(material, m_materialLocations.pLights, m_materialLocations.numPLights))
                {
                    return false;
                }
            }
            else if (material->shaderId == m_terrainShaderId)
            {
                // Apply Properties
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.properties, material->properties));

                // Apply Maps
                for (u32 i = 0; i < TERRAIN_MAX_MATERIAL_COUNT * TERRAIN_PER_MATERIAL_SAMP_COUNT; i++)
                {
                    MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_terrainLocations.samplers[i], &material->maps[i]));
                }

                // Apply shadow map
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
                {
                    material->maps[TERRAIN_SAMP_SHADOW_MAP + i].texture = m_currentShadowTexture[i];
                }

                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.shadowTexture_0, &material->maps[TERRAIN_SAMP_SHADOW_MAP + 0]));
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.shadowTexture_1, &material->maps[TERRAIN_SAMP_SHADOW_MAP + 1]));
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.shadowTexture_2, &material->maps[TERRAIN_SAMP_SHADOW_MAP + 2]));
                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.shadowTexture_3, &material->maps[TERRAIN_SAMP_SHADOW_MAP + 3]));

                // Apply irradiance map
                material->maps[TERRAIN_SAMP_IRRADIANCE_MAP].texture =
                    material->irradianceTexture ? material->irradianceTexture : m_currentIrradianceTexture;

                MATERIAL_APPLY_OR_FAIL(
                    Shaders.SetUniformByIndex(m_terrainLocations.iblCubeTexture, &material->maps[TERRAIN_SAMP_IRRADIANCE_MAP]));

                if (!ApplyPointLights(material, m_terrainLocations.pLights, m_terrainLocations.numPLights))
                {
                    return false;
                }
            }
            else if (material->shaderId == m_pbrShaderId)
            {
                // PBR Shader
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.properties, material->properties));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.albedoTexture, &material->maps[SAMP_ALBEDO]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.normalTexture, &material->maps[SAMP_NORMAL]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.metallicTexture, &material->maps[SAMP_METALLIC]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.roughnessTexture, &material->maps[SAMP_ROUGHNESS]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.aoTexture, &material->maps[SAMP_AO]));

                // Shadow map
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
                {
                    material->maps[PBR_SAMP_SHADOW_MAP_0 + i].texture = m_currentShadowTexture[i];
                }

                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.shadowTexture_0, &material->maps[PBR_SAMP_SHADOW_MAP_0]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.shadowTexture_1, &material->maps[PBR_SAMP_SHADOW_MAP_1]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.shadowTexture_2, &material->maps[PBR_SAMP_SHADOW_MAP_2]));
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.shadowTexture_3, &material->maps[PBR_SAMP_SHADOW_MAP_3]));

                // Irradiance map
                material->maps[PBR_SAMP_IBL_CUBE].texture =
                    material->irradianceTexture ? material->irradianceTexture : m_currentIrradianceTexture;
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.iblCubeTexture, &material->maps[PBR_SAMP_IBL_CUBE]));

                const auto dirLight = Lights.GetDirectionalLight();
                MATERIAL_APPLY_OR_FAIL(Shaders.SetUniformByIndex(m_pbrLocations.dirLight, &dirLight->data));

                if (!ApplyPointLights(material, m_pbrLocations.pLights, m_pbrLocations.numPLights))
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

        MATERIAL_APPLY_OR_FAIL(Shaders.ApplyInstance(needsUpdate))
        return true;
    }

    bool MaterialSystem::ApplyLocal(Material* material, const mat4* model) const
    {
        if (material->shaderId == m_materialShaderId)
        {
            return Shaders.SetUniformByIndex(m_materialLocations.model, model);
        }
        if (material->shaderId == m_pbrShaderId)
        {
            return Shaders.SetUniformByIndex(m_pbrLocations.model, model);
        }
        if (material->shaderId == m_terrainShaderId)
        {
            return Shaders.SetUniformByIndex(m_terrainLocations.model, model);
        }

        ERROR_LOG("Unrecognized shader id: '{}' on material: '{}'.", material->shaderId, material->name);
        return false;
    }

    Material* MaterialSystem::GetDefault()
    {
        if (!m_initialized)
        {
            FATAL_LOG("Tried to get the default Material before system is initialized.");
            return nullptr;
        }
        return &m_defaultMaterial;
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

    bool MaterialSystem::CreateDefaultMaterial()
    {
        m_defaultMaterial.name           = DEFAULT_MATERIAL_NAME;
        m_defaultMaterial.type           = MaterialType::Phong;
        m_defaultMaterial.propertiesSize = sizeof(MaterialPhongProperties);
        m_defaultMaterial.properties     = Memory.Allocate<MaterialPhongProperties>(MemoryType::MaterialInstance);

        auto props          = static_cast<MaterialPhongProperties*>(m_defaultMaterial.properties);
        props->diffuseColor = vec4(1);
        props->shininess    = 8.0f;

        m_defaultMaterial.maps.Resize(3);
        m_defaultMaterial.maps[0].texture = Textures.GetDefaultDiffuse();
        m_defaultMaterial.maps[1].texture = Textures.GetDefaultSpecular();
        m_defaultMaterial.maps[2].texture = Textures.GetDefaultNormal();

        const TextureMap* maps[3] = { &m_defaultMaterial.maps[0], &m_defaultMaterial.maps[1], &m_defaultMaterial.maps[2] };

        Shader* shader = Shaders.Get("Shader.Builtin.Material");
        if (!Renderer.AcquireShaderInstanceResources(*shader, 3, maps, &m_defaultMaterial.internalId))
        {
            ERROR_LOG("Failed to acquire renderer resources for the Default Material.");
            return false;
        }

        // Assign the shader id to the default material
        m_defaultMaterial.shaderId = shader->id;
        return true;
    }

    bool MaterialSystem::CreateDefaultTerrainMaterial()
    {
        m_defaultTerrainMaterial.name           = DEFAULT_TERRAIN_MATERIAL_NAME;
        m_defaultTerrainMaterial.type           = MaterialType::Terrain;
        m_defaultTerrainMaterial.propertiesSize = sizeof(MaterialTerrainProperties);
        m_defaultTerrainMaterial.properties     = Memory.Allocate<MaterialTerrainProperties>(MemoryType::MaterialInstance);

        auto props                       = static_cast<MaterialTerrainProperties*>(m_defaultTerrainMaterial.properties);
        props->numMaterials              = 1;
        props->materials[0].diffuseColor = vec4(1);
        props->materials[0].shininess    = 8.0f;
        props->materials[0].padding      = vec3(0);

        m_defaultTerrainMaterial.maps.Resize(TERRAIN_SAMP_COUNT_TOTAL);
        m_defaultTerrainMaterial.maps[SAMP_ALBEDO].texture    = Textures.GetDefaultDiffuse();
        m_defaultTerrainMaterial.maps[SAMP_NORMAL].texture    = Textures.GetDefaultNormal();
        m_defaultTerrainMaterial.maps[SAMP_METALLIC].texture  = Textures.GetDefaultMetallic();
        m_defaultTerrainMaterial.maps[SAMP_ROUGHNESS].texture = Textures.GetDefaultRoughness();
        m_defaultTerrainMaterial.maps[SAMP_AO].texture        = Textures.GetDefaultAo();

        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + i].texture = Textures.GetDefaultDiffuse();
            m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + i].repeatU = TextureRepeat::ClampToBorder;
            m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + i].repeatV = TextureRepeat::ClampToBorder;
            m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + i].repeatW = TextureRepeat::ClampToBorder;
        }

        const TextureMap* maps[9] = { &m_defaultTerrainMaterial.maps[SAMP_ALBEDO],
                                      &m_defaultTerrainMaterial.maps[SAMP_NORMAL],
                                      &m_defaultTerrainMaterial.maps[SAMP_METALLIC],
                                      &m_defaultTerrainMaterial.maps[SAMP_ROUGHNESS],
                                      &m_defaultTerrainMaterial.maps[SAMP_AO],
                                      &m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + 0],
                                      &m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + 1],
                                      &m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + 2],
                                      &m_defaultTerrainMaterial.maps[TERRAIN_SAMP_SHADOW_MAP + 3] };

        Shader* shader = Shaders.Get("Shader.Builtin.Terrain");
        if (!Renderer.AcquireShaderInstanceResources(*shader, 9, maps, &m_defaultTerrainMaterial.internalId))
        {
            ERROR_LOG("Failed to acquire renderer resources for the Default Terrain Material.");
            return false;
        }

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

        m_defaultPbrMaterial.maps.Resize(PBR_MATERIAL_MAP_COUNT);
        m_defaultPbrMaterial.maps[SAMP_ALBEDO].texture       = Textures.GetDefaultDiffuse();
        m_defaultPbrMaterial.maps[SAMP_NORMAL].texture       = Textures.GetDefaultNormal();
        m_defaultPbrMaterial.maps[SAMP_METALLIC].texture     = Textures.GetDefaultMetallic();
        m_defaultPbrMaterial.maps[SAMP_ROUGHNESS].texture    = Textures.GetDefaultRoughness();
        m_defaultPbrMaterial.maps[SAMP_AO].texture           = Textures.GetDefaultAo();
        m_defaultPbrMaterial.maps[PBR_SAMP_IBL_CUBE].texture = Textures.GetDefaultCube();

        // Change the clamp mode for the default shadow maps to border
        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_0 + i].texture = Textures.GetDefaultDiffuse();
            m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_0 + i].repeatU = TextureRepeat::ClampToBorder;
            m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_0 + i].repeatV = TextureRepeat::ClampToBorder;
            m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_0 + i].repeatW = TextureRepeat::ClampToBorder;
        }

        const TextureMap* maps[PBR_MATERIAL_MAP_COUNT] = { &m_defaultPbrMaterial.maps[SAMP_ALBEDO],
                                                           &m_defaultPbrMaterial.maps[SAMP_NORMAL],
                                                           &m_defaultPbrMaterial.maps[SAMP_METALLIC],
                                                           &m_defaultPbrMaterial.maps[SAMP_ROUGHNESS],
                                                           &m_defaultPbrMaterial.maps[SAMP_AO],
                                                           &m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_0],
                                                           &m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_1],
                                                           &m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_2],
                                                           &m_defaultPbrMaterial.maps[PBR_SAMP_SHADOW_MAP_3],
                                                           &m_defaultPbrMaterial.maps[PBR_SAMP_IBL_CUBE] };

        Shader* shader = Shaders.Get("Shader.PBR");
        if (!Renderer.AcquireShaderInstanceResources(*shader, PBR_MATERIAL_MAP_COUNT, maps, &m_defaultPbrMaterial.internalId))
        {
            ERROR_LOG("Failed to acquire renderer resources for the Default PBR Material.");
            return false;
        }

        // Assign the shader id to the default material
        m_defaultPbrMaterial.shaderId = shader->id;
        return true;
    }

    bool MaterialSystem::AssignMap(TextureMap& map, const MaterialConfigMap& config, Texture* defaultTexture) const
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

        if (config.type == MaterialType::Phong)
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

            // For Phong materials we expect 3 maps (diffuse, specular and normal)
            mat.maps.Resize(3);
            mapCount = 3;

            bool diffuseAssigned  = false;
            bool specularAssigned = false;
            bool normalAssigned   = false;

            for (const auto& map : config.maps)
            {
                if (map.name.IEquals("diffuse"))
                {
                    if (!AssignMap(mat.maps[0], map, Textures.GetDefaultDiffuse()))
                    {
                        return false;
                    }
                    diffuseAssigned = true;
                }
                else if (map.name.IEquals("specular"))
                {
                    if (!AssignMap(mat.maps[1], map, Textures.GetDefaultSpecular()))
                    {
                        return false;
                    }
                    specularAssigned = true;
                }
                else if (map.name.IEquals("normal"))
                {
                    if (!AssignMap(mat.maps[2], map, Textures.GetDefaultNormal()))
                    {
                        return false;
                    }
                    normalAssigned = true;
                }
            }

            if (!diffuseAssigned)
            {
                MaterialConfigMap map("diffuse", "");
                if (!AssignMap(mat.maps[0], map, Textures.GetDefaultDiffuse()))
                {
                    return false;
                }
            }

            if (!specularAssigned)
            {
                MaterialConfigMap map("specular", "");
                if (!AssignMap(mat.maps[1], map, Textures.GetDefaultSpecular()))
                {
                    return false;
                }
            }

            if (!normalAssigned)
            {
                MaterialConfigMap map("normal", "");
                if (!AssignMap(mat.maps[2], map, Textures.GetDefaultNormal()))
                {
                    return false;
                }
            }
        }
        else if (config.type == MaterialType::PBR)
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

            // For PBR materials we expect 5 maps (albedo, normal, metallic, roughness and ao)
            mat.maps.Resize(PBR_MATERIAL_MAP_COUNT);
            mapCount = PBR_MATERIAL_MAP_COUNT;

            bool albedoAssigned    = false;
            bool normalAssigned    = false;
            bool metallicAssigned  = false;
            bool roughnessAssigned = false;
            bool aoAssigned        = false;
            bool cubeAssigned      = false;

            for (const auto& map : config.maps)
            {
                if (map.name.IEquals("albedo"))
                {
                    if (!AssignMap(mat.maps[SAMP_ALBEDO], map, Textures.GetDefaultDiffuse()))
                    {
                        return false;
                    }
                    albedoAssigned = true;
                }
                else if (map.name.IEquals("normal"))
                {
                    if (!AssignMap(mat.maps[SAMP_NORMAL], map, Textures.GetDefaultNormal()))
                    {
                        return false;
                    }
                    normalAssigned = true;
                }
                else if (map.name.IEquals("metallic"))
                {
                    if (!AssignMap(mat.maps[SAMP_METALLIC], map, Textures.GetDefaultMetallic()))
                    {
                        return false;
                    }
                    metallicAssigned = true;
                }
                else if (map.name.IEquals("roughness"))
                {
                    if (!AssignMap(mat.maps[SAMP_ROUGHNESS], map, Textures.GetDefaultRoughness()))
                    {
                        return false;
                    }
                    roughnessAssigned = true;
                }
                else if (map.name.IEquals("ao"))
                {
                    if (!AssignMap(mat.maps[SAMP_AO], map, Textures.GetDefaultAo()))
                    {
                        return false;
                    }
                    aoAssigned = true;
                }
                else if (map.name.IEquals("iblCube"))
                {
                    if (!AssignMap(mat.maps[PBR_SAMP_IBL_CUBE], map, Textures.GetDefaultCube()))
                    {
                        return false;
                    }
                    cubeAssigned = true;
                }
            }

            // Shadow map
            {
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
                {
                    MaterialConfigMap mapConfig;
                    mapConfig.minifyFilter  = TextureFilter::ModeLinear;
                    mapConfig.magnifyFilter = TextureFilter::ModeLinear;
                    mapConfig.repeatU       = TextureRepeat::ClampToBorder;
                    mapConfig.repeatV       = TextureRepeat::ClampToBorder;
                    mapConfig.repeatW       = TextureRepeat::ClampToBorder;
                    mapConfig.name          = "shadowMap";
                    mapConfig.textureName   = "";
                    if (!AssignMap(mat.maps[PBR_SAMP_SHADOW_MAP_0 + i], mapConfig, Textures.GetDefaultDiffuse()))
                    {
                        return false;
                    }
                }
            }

            if (!albedoAssigned)
            {
                MaterialConfigMap map("albedo", "");
                if (!AssignMap(mat.maps[SAMP_ALBEDO], map, Textures.GetDefaultDiffuse()))
                {
                    return false;
                }
            }

            if (!normalAssigned)
            {
                MaterialConfigMap map("normal", "");
                if (!AssignMap(mat.maps[SAMP_NORMAL], map, Textures.GetDefaultNormal()))
                {
                    return false;
                }
            }

            if (!metallicAssigned)
            {
                MaterialConfigMap map("metallic", "");
                if (!AssignMap(mat.maps[SAMP_METALLIC], map, Textures.GetDefaultMetallic()))
                {
                    return false;
                }
            }

            if (!roughnessAssigned)
            {
                MaterialConfigMap map("roughness", "");
                if (!AssignMap(mat.maps[SAMP_ROUGHNESS], map, Textures.GetDefaultRoughness()))
                {
                    return false;
                }
            }

            if (!aoAssigned)
            {
                MaterialConfigMap map("ao", "");
                if (!AssignMap(mat.maps[SAMP_AO], map, Textures.GetDefaultAo()))
                {
                    return false;
                }
            }

            if (!cubeAssigned)
            {
                MaterialConfigMap map("iblCube", "");
                if (!AssignMap(mat.maps[PBR_SAMP_IBL_CUBE], map, Textures.GetDefaultCube()))
                {
                    return false;
                }
            }
        }
        else if (config.type == MaterialType::UI)
        {
            // NOTE: UI's only have one map and one property so we only use those
            // TODO: If this changes we need to make sure we handle it here properly
            mat.maps.Resize(1);
            mapCount = 1;

            mat.propertiesSize = sizeof(MaterialUIProperties);
            mat.properties     = Memory.Allocate<MaterialUIProperties>(MemoryType::MaterialInstance);
            // Defaults
            auto props          = static_cast<MaterialUIProperties*>(mat.properties);
            props->diffuseColor = std::get<vec4>(config.props[0].value);
            if (!AssignMap(mat.maps[0], config.maps[0], Textures.GetDefaultDiffuse()))
            {
                return false;
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

        Shader* shader = nullptr;
        switch (config.type)
        {
            case MaterialType::Phong:
                shader = Shaders.Get(config.shaderName.Empty() ? "Shader.Builtin.Material" : config.shaderName);
                break;
            case MaterialType::PBR:
                shader = Shaders.Get(config.shaderName.Empty() ? "Shader.PBR" : config.shaderName);
                break;
            case MaterialType::Terrain:
                shader = Shaders.Get(config.shaderName.Empty() ? "Shader.Builtin.Terrain" : config.shaderName);
                break;
            case MaterialType::UI:
                shader = Shaders.Get(config.shaderName.Empty() ? "Shader.Builtin.UI" : config.shaderName);
                break;
            case MaterialType::Custom:
                if (config.shaderName.Empty())
                {
                    FATAL_LOG("Custom Material: '{}' does not have a Shader name which is required.", config.name);
                    return false;
                }
                shader = Shaders.Get(config.shaderName);
                break;
            default:
                ERROR_LOG("Unsupported Material type: '{}'.", ToString(config.type));
                return false;
        }

        if (!shader)
        {
            ERROR_LOG("Failed to load Material because its shader was not found.");
            return false;
        }

        // Gather a list of pointers to our texture maps
        const TextureMap** maps = Memory.Allocate<const TextureMap*>(MemoryType::Array, mapCount);
        for (u32 i = 0; i < mapCount; i++)
        {
            maps[i] = &mat.maps[i];
        }

        bool result = Renderer.AcquireShaderInstanceResources(*shader, mapCount, maps, &mat.internalId);
        if (!result)
        {
            ERROR_LOG("Failed to acquire renderer resources for Material: '{}'.", mat.name);
        }

        if (maps)
        {
            Memory.Free(maps);
        }

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
                Textures.Release(map.texture->name);
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
