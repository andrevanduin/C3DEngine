
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

    constexpr auto PBR_MATERIAL_MAP_COUNT = 7;
    constexpr auto PBR_SAMP_ALBEDO        = 0;
    constexpr auto PBR_SAMP_NORMAL        = 1;
    constexpr auto PBR_SAMP_METALLIC      = 2;
    constexpr auto PBR_SAMP_ROUGHNESS     = 3;
    constexpr auto PBR_SAMP_AO            = 4;
    constexpr auto PBR_SAMP_SHADOW_MAP    = 5;
    constexpr auto PBR_SAMP_IBL_CUBE      = 6;

    constexpr auto TERRAIN_PER_MATERIAL_SAMP_COUNT = 5;
    constexpr auto TERRAIN_SAMP_COUNT_TOTAL        = 2 + (TERRAIN_PER_MATERIAL_SAMP_COUNT * TERRAIN_MAX_MATERIAL_COUNT);
    constexpr auto TERRAIN_SAMP_SHADOW_MAP         = TERRAIN_PER_MATERIAL_SAMP_COUNT * TERRAIN_MAX_MATERIAL_COUNT;
    constexpr auto TERRAIN_SAMP_IRRADIANCE_MAP     = TERRAIN_SAMP_SHADOW_MAP + 1;

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
        u16 ambientColor = INVALID_ID_U16;
        u16 viewPosition = INVALID_ID_U16;
        u16 model        = INVALID_ID_U16;
        u16 renderMode   = INVALID_ID_U16;
        u16 dirLight     = INVALID_ID_U16;
        u16 pLights      = INVALID_ID_U16;
        u16 numPLights   = INVALID_ID_U16;

        u16 properties     = INVALID_ID_U16;
        u16 iblCubeTexture = INVALID_ID_U16;
        u16 shadowTexture  = INVALID_ID_U16;
        u16 lightSpace     = INVALID_ID_U16;
        u16 samplers[TERRAIN_MAX_MATERIAL_COUNT * 5];  // Albedo, normal, metallic, roughness, ao
        u16 usePCF = INVALID_ID_U16;
        u16 bias   = INVALID_ID_U16;
    };

    struct PbrUniformLocations
    {
        u16 projection       = INVALID_ID_U16;
        u16 view             = INVALID_ID_U16;
        u16 ambientColor     = INVALID_ID_U16;
        u16 viewPosition     = INVALID_ID_U16;
        u16 properties       = INVALID_ID_U16;
        u16 iblCubeTexture   = INVALID_ID_U16;
        u16 albedoTexture    = INVALID_ID_U16;
        u16 normalTexture    = INVALID_ID_U16;
        u16 metallicTexture  = INVALID_ID_U16;
        u16 roughnessTexture = INVALID_ID_U16;
        u16 aoTexture        = INVALID_ID_U16;
        u16 shadowTexture    = INVALID_ID_U16;
        u16 lightSpace       = INVALID_ID_U16;
        u16 model            = INVALID_ID_U16;
        u16 renderMode       = INVALID_ID_U16;
        u16 usePCF           = INVALID_ID_U16;
        u16 bias             = INVALID_ID_U16;
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

        /**
         * @brief Sets the provided shadowmap texture to be used for all future draw calls.
         *
         * @param shadowTexture The texture that should be used as shadow map
         * @param index - Not used yet (for cascading)
         * @return True if successful; false otherwise
         */
        bool SetShadowMap(Texture* shadowTexture, u8 index);

        /**
         * @brief Sets the Directional light-space matrix to be used for future draw calls.
         *
         * @param lightSpace The directional light-space matrix
         */
        void SetDirectionalLightSpaceMatrix(const mat4& lightSpace);

        bool ApplyGlobal(u32 shaderId, const FrameData& frameData, const mat4* projection, const mat4* view, const vec4* ambientColor,
                         const vec3* viewPosition, u32 renderMode) const;
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
        bool ApplyPointLights(Material* material, u16 pLightsLoc, u16 numPLightsLoc) const;

        bool LoadMaterial(const MaterialConfig& config, Material& mat) const;

        void DestroyMaterial(Material& mat) const;

        Material m_defaultMaterial, m_defaultTerrainMaterial, m_defaultPbrMaterial;
        /** @brief HashMap to map names to material-references */
        HashMap<String, MaterialReference> m_registeredMaterials;

        /** @brief Current irradiance and shadow textures. */
        Texture* m_currentIrradianceTexture = nullptr;
        Texture* m_currentShadowTexture     = nullptr;

        mat4 m_directionalLightSpace = mat4(1.0f);

        // Known locations for the Material Shader
        MaterialUniformLocations m_materialLocations;
        u32 m_materialShaderId = INVALID_ID;

        // Known locations for the Terrain Shader
        TerrainUniformLocations m_terrainLocations;
        u32 m_terrainShaderId = INVALID_ID;

        // Known locations for the PBR Shader
        PbrUniformLocations m_pbrLocations;
        u32 m_pbrShaderId = INVALID_ID;

        // Flag indicating if we should be using PCF
        i32 m_usePCF = 1;
    };
}  // namespace C3D
