
#pragma once
#include "containers/hash_map.h"
#include "containers/hash_table.h"
#include "defines.h"
#include "logger/logger.h"
#include "resources/materials/material.h"
#include "resources/resource_types.h"
#include "systems/system.h"

namespace C3D
{
    class FrameData;
    struct DirectionalLightData;
    struct PointLightData;

    constexpr auto MATERIAL_SYSTEM_DEFAULT_MAX_MATERIALS = 128;

    constexpr auto DEFAULT_TERRAIN_MATERIAL_NAME = "default_terrain";
    constexpr auto DEFAULT_PBR_MATERIAL_NAME     = "default_pbr";

    constexpr auto PBR_SAMP_ALBEDO         = 0;
    constexpr auto PBR_SAMP_NORMAL         = 1;
    constexpr auto PBR_SAMP_COMBINED       = 2;
    constexpr auto PBR_SAMP_SHADOW_MAP     = 3;
    constexpr auto PBR_SAMP_IRRADIANCE_MAP = 4;

    constexpr auto PBR_TOTAL_MAP_COUNT        = 5;
    constexpr auto PBR_MATERIAL_TEXTURE_COUNT = 3;

    constexpr auto MAX_SHADOW_CASCADE_COUNT = 4;

    constexpr auto TERRAIN_SAMP_MATERIALS      = 0;  // sampler2DArray of all material textures
    constexpr auto TERRAIN_SAMP_SHADOW_MAP     = 1;  // sampler2DArray of all shadow textures (MAX_SHADOW_CASCADE_COUNT)
    constexpr auto TERRAIN_SAMP_IRRADIANCE_MAP = 2;  // sampler2D of the irradiance map
    constexpr auto TERRAIN_TOTAL_MAP_COUNT     = 3;

    struct MaterialSystemConfig
    {
        u32 maxMaterials = MATERIAL_SYSTEM_DEFAULT_MAX_MATERIALS;
    };

    struct MaterialReference
    {
        MaterialReference(bool shouldAutoRelease, u32 index) : autoRelease(shouldAutoRelease) { material.id = index; }

        Material material;
        u32 referenceCount = 1;
        bool autoRelease   = false;
    };

    struct TerrainUniformLocations
    {
        u16 projection    = INVALID_ID_U16;
        u16 view          = INVALID_ID_U16;
        u16 cascadeSplits = INVALID_ID_U16;
        u16 viewPosition  = INVALID_ID_U16;
        u16 model         = INVALID_ID_U16;
        u16 renderMode    = INVALID_ID_U16;
        u16 dirLight      = INVALID_ID_U16;
        u16 pLights       = INVALID_ID_U16;
        u16 numPLights    = INVALID_ID_U16;

        u16 properties       = INVALID_ID_U16;
        u16 materialTextures = INVALID_ID_U16;
        u16 shadowTextures   = INVALID_ID_U16;
        u16 iblCubeTexture   = INVALID_ID_U16;
        u16 lightSpaces      = INVALID_ID_U16;
        u16 usePCF           = INVALID_ID_U16;
        u16 bias             = INVALID_ID_U16;
    };

    struct PbrUniformLocations
    {
        u16 projection       = INVALID_ID_U16;
        u16 view             = INVALID_ID_U16;
        u16 cascadeSplits    = INVALID_ID_U16;
        u16 viewPosition     = INVALID_ID_U16;
        u16 properties       = INVALID_ID_U16;
        u16 materialTextures = INVALID_ID_U16;
        u16 shadowTextures   = INVALID_ID_U16;
        u16 iblCubeTexture   = INVALID_ID_U16;
        u16 lightSpaces      = INVALID_ID_U16;
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
         * @param handle A handle to the texture you want to use as an irradiance texture
         * @return True if successful; false otherwise
         */
        bool SetIrradiance(TextureHandle handle);

        /** @brief Reset the current irradiance cubemap texture to the default. */
        void ResetIrradiance();

        /**
         * @brief Sets the provided shadowmap texture to be used for all future draw calls.
         *
         * @param handle handle to the texture that should be used as shadow map
         * @param cascadeIndex The cascade index (level) (between 0 and 4)
         */
        void SetShadowMap(TextureHandle handle, u8 cascadeIndex);

        /**
         * @brief Sets the Directional light-space matrix to be used for future draw calls.
         *
         * @param lightSpace The directional light-space matrix
         * @param index The cascade index for which you want to set the light-space matrix
         */
        void SetDirectionalLightSpaceMatrix(const mat4& lightSpace, u8 index);

        bool ApplyGlobal(u32 shaderId, const FrameData& frameData, const DirectionalLightData& dirLight, const mat4* projection,
                         const mat4* view, const vec4* cascadeSplits, const vec3* viewPosition, u32 renderMode) const;
        bool ApplyInstance(Material* material, const DirectionalLightData& dirLight,
                           const DynamicArray<PointLightData, LinearAllocator>& pointLights, const FrameData& frameData,
                           bool needsUpdate) const;
        bool ApplyLocal(const FrameData& frameData, Material* material, const mat4* model) const;

        Material* GetDefault();
        Material* GetDefaultTerrain();
        Material* GetDefaultPbr();

    private:
        bool CreateDefaultTerrainMaterial();
        bool CreateDefaultPbrMaterial();

        Material& AcquireReference(const String& name, bool autoRelease, bool& needsCreation);

        bool AssignMap(TextureMap& map, const MaterialConfigMap& config, TextureHandle defaultTexture) const;
        bool ApplyPointLights(Material* material, const DynamicArray<PointLightData, LinearAllocator>& pointLights, u16 pLightsLoc,
                              u16 numPLightsLoc) const;

        bool LoadMaterial(const MaterialConfig& config, Material& mat) const;

        void DestroyMaterial(Material& mat) const;

        Material m_defaultTerrainMaterial, m_defaultPbrMaterial;

        /** @brief HashMap to map names to material-references */
        HashMap<String, u32> m_nameToMaterialIndexMap;
        /** @brief An array used to store our MaterialReferences */
        DynamicArray<MaterialReference> m_materials;

        /** @brief Current irradiance and shadow textures. */
        TextureHandle m_currentIrradianceTexture = INVALID_ID;
        TextureHandle m_currentShadowTexture     = INVALID_ID;

        mat4 m_directionalLightSpace[MAX_SHADOW_CASCADE_COUNT] = { mat4(1.0f), mat4(1.0f), mat4(1.0f), mat4(1.0f) };

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
