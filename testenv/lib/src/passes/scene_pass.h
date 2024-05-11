
#pragma once
#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <memory/allocators/linear_allocator.h>
#include <renderer/renderer_types.h>
#include <renderer/rendergraph/renderpass.h>

#include "test_env_types.h"

namespace C3D
{
    class Shader;
    class Camera;
    class DebugLine3D;
    class DebugBox3D;
    class Viewport;
    class Camera;
}  // namespace C3D

class SimpleScene;

class ScenePass : public C3D::Renderpass
{
public:
    ScenePass();
    ScenePass(const C3D::SystemManager* pSystemsManager);

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool LoadResources() override;
    bool Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::FrameData& frameData, const SimpleScene& scene, u32 renderMode,
                 const C3D::DynamicArray<C3D::DebugLine3D>& debugLines, const C3D::DynamicArray<C3D::DebugBox3D>& debugBoxes,
                 const mat4& shadowCameraLookat, const mat4& shadowCameraProjection);
    bool Execute(const C3D::FrameData& frameData) override;

private:
    C3D::Shader* m_shader        = nullptr;
    C3D::Shader* m_terrainShader = nullptr;
    C3D::Shader* m_colorShader   = nullptr;
    C3D::Shader* m_pbrShader     = nullptr;

    C3D::RendergraphSource* m_shadowMapSource;
    C3D::DynamicArray<C3D::TextureMap> m_shadowMaps;

    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_geometries;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_terrains;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_debugGeometries;
    C3D::DynamicArray<GeometryDistance, C3D::LinearAllocator> m_transparentGeometries;

    C3D::Texture* m_irradianceCubeTexture = nullptr;

    mat4 m_directionalLightView, m_directionalLightProjection;

    u32 m_renderMode;

    DebugColorShaderLocations m_debugLocations;
};