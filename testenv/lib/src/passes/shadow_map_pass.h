
#pragma once
#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <memory/allocators/linear_allocator.h>
#include <renderer/renderer_types.h>
#include <renderer/rendergraph/renderpass.h>
#include <renderer/viewport.h>
#include <resources/textures/texture_map.h>
#include <systems/lights/light_system.h>

#include "test_env_types.h"

namespace C3D
{
    class Shader;
    class Camera;
    class DebugLine3D;
    class DebugBox3D;
    class Camera;
}  // namespace C3D

class SimpleScene;

struct ShadowMapPassConfig
{
    u16 resolution;
    f32 nearClip;
    f32 farClip;
    C3D::RendererProjectionMatrixType matrixType;
    C3D::Rect2D bounds;
    f32 fov;
};

struct ShadowMapShaderLocations
{
    u16 projection = INVALID_ID_U16;
    u16 view       = INVALID_ID_U16;
    u16 model      = INVALID_ID_U16;
    u16 colorMap   = INVALID_ID_U16;
};

class ShadowMapPass : public C3D::Renderpass
{
public:
    ShadowMapPass();
    ShadowMapPass(const C3D::SystemManager* pSystemsManager, const ShadowMapPassConfig& config);

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool LoadResources() override;
    bool Prepare(const SimpleScene& scene);
    bool Execute(const C3D::FrameData& frameData) override;
    void Destroy() override;

    C3D::Texture* GetAttachmentTexture(C3D::RenderTargetAttachmentType type, u8 frameNumber) override;

    const mat4& GetShadowCameraLookat() const { return m_viewMatrix; }
    const mat4& GetShadowCameraProjection() const { return m_projectionMatrix; }

private:
    ShadowMapPassConfig m_config;

    C3D::Shader* m_shader        = nullptr;
    C3D::Shader* m_terrainShader = nullptr;

    ShadowMapShaderLocations m_locations, m_terrainLocations;

    mat4 m_projectionMatrix;
    mat4 m_viewMatrix;

    C3D::Viewport m_viewport;

    C3D::DynamicArray<C3D::Texture> m_depthTextures;
    C3D::DynamicArray<C3D::Texture> m_colorTextures;

    C3D::DynamicArray<bool, C3D::LinearAllocator> m_instanceUpdated;
    u32 m_instanceCount = 0;

    C3D::TextureMap m_defaultColorMap, m_defaultTerrainColorMap;
    u32 m_defaultInstanceId, m_terrainInstanceId;
    u64 m_defaultInstanceFrameNumber, m_terrainInstanceFrameNumber;
    u8 m_defaultInstanceDrawIndex, m_terrainInstanceDrawIndex;

    const C3D::DirectionalLight* m_directionalLight = nullptr;

    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_geometries;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_terrains;
    C3D::DynamicArray<GeometryDistance, C3D::LinearAllocator> m_transparentGeometries;
};