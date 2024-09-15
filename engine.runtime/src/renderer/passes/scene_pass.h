
#pragma once
#include "containers/dynamic_array.h"
#include "defines.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/passes/shadow_map_pass.h"
#include "renderer/renderer_types.h"
#include "renderer/rendergraph/renderpass.h"
#include "resources/debug/debug_types.h"

namespace C3D
{
    class Shader;
    class Camera;
    class DebugLine3D;
    class DebugBox3D;
    class Viewport;
    class Camera;

    class Scene;

    class C3D_API ScenePass : public Renderpass
    {
    public:
        ScenePass();

        bool Initialize(const LinearAllocator* frameAllocator) override;
        bool LoadResources() override;
        bool Prepare(const Viewport& viewport, Camera* camera, FrameData& frameData, Scene& scene, u32 renderMode,
                     const DynamicArray<DebugLine3D>& debugLines, const DynamicArray<DebugBox3D>& debugBoxes,
                     ShadowMapCascadeData* cascadeData);
        bool Execute(const FrameData& frameData) override;
        void Destroy() override;

    private:
        Shader* m_pbrShader     = nullptr;
        Shader* m_terrainShader = nullptr;
        Shader* m_colorShader   = nullptr;

        const RendergraphSource* m_shadowMapSource = nullptr;
        DynamicArray<C3D::TextureMap> m_shadowMaps;

        vec4 m_cascadeSplits;

        DynamicArray<GeometryRenderData, LinearAllocator> m_geometries;
        DynamicArray<GeometryRenderData, LinearAllocator> m_terrains;
        DynamicArray<GeometryRenderData, LinearAllocator> m_debugGeometries;
        DynamicArray<PointLightData, LinearAllocator> m_pointLights;
        DynamicArray<DirectionalLightData, LinearAllocator> m_directionalLights;

        TextureHandle m_irradianceCubeTexture = INVALID_ID;

        mat4 m_directionalLightViews[4];
        mat4 m_directionalLightProjections[4];

        u32 m_renderMode;

        C3D::DebugColorShaderLocations m_debugLocations;
    };

}  // namespace C3D
