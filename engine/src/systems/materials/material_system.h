
#pragma once
#include "containers/hash_map.h"
#include "containers/hash_table.h"
#include "core/defines.h"
#include "core/logger.h"
#include "resources/materials/material.h"
#include "resources/resource_types.h"
#include "systems/system.h"

namespace C3D
{
    class FrameData;

    constexpr auto DEFAULT_MATERIAL_NAME         = "default";
    constexpr auto DEFAULT_UI_MATERIAL_NAME      = "default_ui";
    constexpr auto DEFAULT_TERRAIN_MATERIAL_NAME = "default_terrain";
    constexpr auto DEFAULT_PBR_MATERIAL_NAME     = "default_pbr";

    constexpr auto PBR_MATERIAL_MAP_COUNT = 6;
    constexpr auto PBR_SAMP_ALBEDO        = 0;
    constexpr auto PBR_SAMP_NORMAL        = 1;
    constexpr auto PBR_SAMP_METALLIC      = 2;
    constexpr auto PBR_SAMP_ROUGHNESS     = 3;
    constexpr auto PBR_SAMP_AO            = 4;
    constexpr auto PBR_SAMP_IBL_CUBE      = 5;

    struct MaterialSystemConfig
    {
        u32 maxMaterialCount;
    };

    struct MaterialReference
    {
        MaterialReference(bool shouldAutoRelease) : autoRelease(shouldAutoRelease) {}

        u32 referenceCount = 1;
        bool autoRelease;
        Material material;
    };

    struct MaterialUniformLocations
    {
        u16 projection      = INVALID_ID_U16;
        u16 view            = INVALID_ID_U16;
        u16 ambientColor    = INVALID_ID_U16;
        u16 viewPosition    = INVALID_ID_U16;
        u16 properties      = INVALID_ID_U16;
        u16 diffuseTexture  = INVALID_ID_U16;
        u16 specularTexture = INVALID_ID_U16;
        u16 normalTexture   = INVALID_ID_U16;
        u16 model           = INVALID_ID_U16;
        u16 renderMode      = INVALID_ID_U16;
        u16 dirLight        = INVALID_ID_U16;
        u16 pLights         = INVALID_ID_U16;
        u16 numPLights      = INVALID_ID_U16;
    };

    struct TerrainUniformLocations
    {
        u16 projection   = INVALID_ID_U16;
        u16 view         = INVALID_ID_U16;
        u16 viewPosition = INVALID_ID_U16;
        u16 model        = INVALID_ID_U16;
        u16 renderMode   = INVALID_ID_U16;
        u16 dirLight     = INVALID_ID_U16;
        u16 pLights      = INVALID_ID_U16;
        u16 numPLights   = INVALID_ID_U16;

        u16 properties     = INVALID_ID_U16;
        u16 iblCubeTexture = INVALID_ID_U16;
        u16 samplers[TERRAIN_MAX_MATERIAL_COUNT * 5];  // Albedo, normal, metallic, roughness, ao
    };

    struct PbrUniformLocations
    {
        u16 projection       = INVALID_ID_U16;
        u16 view             = INVALID_ID_U16;
        u16 viewPosition     = INVALID_ID_U16;
        u16 properties       = INVALID_ID_U16;
        u16 iblCubeTexture   = INVALID_ID_U16;
        u16 albedoTexture    = INVALID_ID_U16;
        u16 normalTexture    = INVALID_ID_U16;
        u16 metallicTexture  = INVALID_ID_U16;
        u16 roughnessTexture = INVALID_ID_U16;
        u16 aoTexture        = INVALID_ID_U16;
        u16 model            = INVALID_ID_U16;
        u16 renderMode       = INVALID_ID_U16;
        u16 dirLight         = INVALID_ID_U16;
        u16 pLights          = INVALID_ID_U16;
        u16 numPLights       = INVALID_ID_U16;
    };

    class C3D_API MaterialSystem final : public SystemWithConfig<MaterialSystemConfig>
    {
    public:
        explicit MaterialSystem(const SystemManager* pSystemsManager);

        bool OnInit(const MaterialSystemConfig& config) override;

        void OnShutdown() override;

        Material* Acquire(const String& name);
        Material* AcquireTerrain(const String& name, const DynamicArray<String>& materialNames, bool autoRelease);
        Material* AcquireFromConfig(const MaterialConfig& config);

        void Release(const String& name);

        /**
         * @brief Sets the provided cubemap texture to be used as Irradiance for all future drawing of materials that do not have an
         * explicitly set irradiance texture. This texture must be a cubemap texture, otherwise this operation will fail.
         *
         * @param irradianceCubeTexture The texture you want to use as an irradiance texture
         * @return True if successful; false otherwise
         */
        bool SetIrradiance(Texture* irradianceCubeTexture);

        /** @brief Reset the current irradiance cubemap texture to the default. */
        void ResetIrradiance();

        bool ApplyGlobal(u32 shaderId, const FrameData& frameData, const mat4* projection, const mat4* view, const vec3* viewPosition,
                         u32 renderMode) const;
        bool ApplyInstance(Material* material, const FrameData& frameData, bool needsUpdate) const;
        bool ApplyLocal(Material* material, const mat4* model) const;

        Material* GetDefault();
        Material* GetDefaultTerrain();
        Material* GetDefaultPbr();

    private:
        bool CreateDefaultMaterial();
        bool CreateDefaultTerrainMaterial();
        bool CreateDefaultPbrMaterial();

        Material& AcquireReference(const String& name, bool autoRelease, bool& needsCreation);

        bool AssignMap(TextureMap& map, const MaterialConfigMap& config, Texture* defaultTexture) const;
        bool ApplyLights(Material* material, u16 dirLightLoc, u16 pLightsLoc, u16 numPLightsLoc) const;

        bool LoadMaterial(const MaterialConfig& config, Material& mat) const;

        void DestroyMaterial(Material& mat) const;

        Material m_defaultMaterial, m_defaultTerrainMaterial, m_defaultPbrMaterial;
        /** @brief HashMap to map names to material-references */
        HashMap<String, MaterialReference> m_registeredMaterials;

        Texture* m_currentIrradianceTexture = nullptr;

        // Known locations for the Material Shader
        MaterialUniformLocations m_materialLocations;
        u32 m_materialShaderId = INVALID_ID;

        // Known locations for the Terrain Shader
        TerrainUniformLocations m_terrainLocations;
        u32 m_terrainShaderId = INVALID_ID;

        // Known locations for the PBR Shader
        PbrUniformLocations m_pbrLocations;
        u32 m_pbrShaderId = INVALID_ID;
    };
}  // namespace C3D
