
#pragma once
#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <renderer/render_view_types.h>
#include <renderer/rendergraph/rendergraph_pass.h>

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

class ScenePass : public C3D::RendergraphPass
{
public:
    ScenePass();
    ScenePass(const C3D::SystemManager* pSystemsManager);

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::FrameData& frameData, const SimpleScene& scene, u32 renderMode,
                 const C3D::DynamicArray<C3D::DebugLine3D>& debugLines, const C3D::DynamicArray<C3D::DebugBox3D>& debugBoxes);
    bool Execute(const C3D::FrameData& frameData) override;

private:
    C3D::Shader* m_shader        = nullptr;
    C3D::Shader* m_terrainShader = nullptr;
    C3D::Shader* m_colorShader   = nullptr;

    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_geometries;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_terrains;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_debugGeometries;

    u32 m_renderMode;
    vec4 m_ambientColor;

    DebugColorShaderLocations m_debugLocations;
};