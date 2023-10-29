
#pragma once
#include <unordered_map>

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

    struct UIUniformLocations
    {
        u16 projection     = INVALID_ID_U16;
        u16 view           = INVALID_ID_U16;
        u16 properties     = INVALID_ID_U16;
        u16 diffuseTexture = INVALID_ID_U16;
        u16 model          = INVALID_ID_U16;
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

        u16 properties = INVALID_ID_U16;
        u16 samplers[TERRAIN_MAX_MATERIAL_COUNT * 3];  // Diffuse, Specular and Normal
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

        Material* GetDefault();
        Material* GetDefaultUI();
        Material* GetDefaultTerrain();

        bool ApplyGlobal(u32 shaderId, const FrameData& frameData, const mat4* projection, const mat4* view, const vec4* ambientColor,
                         const vec3* viewPosition, u32 renderMode) const;
        bool ApplyInstance(Material* material, bool needsUpdate) const;
        bool ApplyLocal(Material* material, const mat4* model) const;

    private:
        bool CreateDefaultMaterial();
        bool CreateDefaultUIMaterial();
        bool CreateDefaultTerrainMaterial();

        Material& AcquireReference(const String& name, bool autoRelease, bool& needsCreation);

        bool AssignMap(TextureMap& map, const MaterialConfigMap& config, Texture* defaultTexture) const;
        bool ApplyLights(Material* material, u16 dirLightLoc, u16 pLightsLoc, u16 numPLightsLoc) const;

        bool LoadMaterial(const MaterialConfig& config, Material& mat) const;

        void DestroyMaterial(Material& mat) const;

        Material m_defaultMaterial, m_defaultUIMaterial, m_defaultTerrainMaterial;
        /** @brief HashMap to map names to material-references */
        HashMap<String, MaterialReference> m_registeredMaterials;

        // Known locations for the Material Shader
        MaterialUniformLocations m_materialLocations;
        u32 m_materialShaderId = INVALID_ID;

        // Known locations for the UI Shader
        UIUniformLocations m_uiLocations;
        u32 m_uiShaderId = INVALID_ID;

        // Known locations for the Terrain Shader
        TerrainUniformLocations m_terrainLocations;
        u32 m_terrainShaderId = INVALID_ID;
    };
}  // namespace C3D
