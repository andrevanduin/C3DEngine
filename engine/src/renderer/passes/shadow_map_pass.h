
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/frame_data.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderer_types.h"
#include "renderer/rendergraph/renderpass.h"
#include "renderer/viewport.h"
#include "resources/textures/texture_map.h"
#include "systems/lights/light_system.h"
#include "systems/materials/material_system.h"

namespace C3D
{
    class Shader;
    class Camera;
    class DebugLine3D;
    class DebugBox3D;
    class Camera;

    struct ShadowMapPassConfig
    {
        u16 resolution;
    };

    struct ShadowMapShaderLocations
    {
        u16 projections  = INVALID_ID_U16;
        u16 views        = INVALID_ID_U16;
        u16 model        = INVALID_ID_U16;
        u16 cascadeIndex = INVALID_ID_U16;
        u16 colorMap     = INVALID_ID_U16;
    };

    struct ShadowMapCascadeData
    {
        mat4 projection;
        mat4 view;

        f32 splitDepth   = 0.f;
        i32 cascadeIndex = 0;

        DynamicArray<GeometryRenderData, LinearAllocator> geometries;
        DynamicArray<GeometryRenderData, LinearAllocator> terrains;
    };

    struct CascadeResources
    {
        // One per frame
        RenderTarget* targets = nullptr;
    };

    struct ShadowShaderInstanceData
    {
        u64 frameNumber = INVALID_ID_U64;
        u8 drawIndex    = INVALID_ID_U8;
    };

    struct CullingData
    {
        vec3 lightDirection;
        vec3 center;
        f32 radius;
    };

    class C3D_API ShadowMapPass : public Renderpass
    {
    public:
        ShadowMapPass();
        ShadowMapPass(const SystemManager* pSystemsManager, const String& name, const ShadowMapPassConfig& config);

        bool Initialize(const LinearAllocator* frameAllocator) override;
        bool LoadResources() override;
        bool Prepare(FrameData& frameData, const Viewport& viewport, Camera* camera);
        bool Execute(const FrameData& frameData) override;
        void Destroy() override;

        ShadowMapCascadeData& GetCascadeData(u32 index) { return m_cascadeData[index]; }
        ShadowMapCascadeData* GetCascadeData() { return m_cascadeData; }

        const CullingData& GetCullingData() const { return m_cullingData; }

        bool PopulateSource(RendergraphSource& source);
        bool PopulateAttachment(RenderTargetAttachment& attachment);

    private:
        ShadowMapPassConfig m_config;

        Shader* m_shader        = nullptr;
        Shader* m_terrainShader = nullptr;

        ShadowMapShaderLocations m_locations;
        ShadowMapShaderLocations m_terrainLocations;

        // Custom projection matrix for the shadow pass
        Viewport m_viewport;

        // Depth textures we use to render our shadow map to
        C3D::DynamicArray<C3D::Texture> m_depthTextures;

        // One per cascade
        CascadeResources m_cascades[MAX_SHADOW_CASCADE_COUNT];
        ShadowMapCascadeData m_cascadeData[MAX_SHADOW_CASCADE_COUNT];

        // Data to be used for culling
        CullingData m_cullingData;

        // Track instance updates per frame
        C3D::DynamicArray<ShadowShaderInstanceData> m_instances;
        // Number of instances
        u32 m_instanceCount = 0;

        // Default texture map to use when no material is available
        TextureMap m_defaultColorMap;
        TextureMap m_defaultTerrainColorMap;

        u32 m_defaultInstanceId        = INVALID_ID;
        u32 m_defaultTerrainInstanceId = INVALID_ID;

        u64 m_defaultInstanceFrameNumber        = INVALID_ID_U64;
        u64 m_defaultTerrainInstanceFrameNumber = INVALID_ID_U64;

        u8 m_defaultInstanceDrawIndex        = INVALID_ID_U8;
        u8 m_defaultTerrainInstanceDrawIndex = INVALID_ID_U8;
    };

}  // namespace C3D