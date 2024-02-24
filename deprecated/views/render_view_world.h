
#pragma once
#include <core/events/event_context.h>
#include <renderer/render_view.h>

#include "test_env_types.h"

namespace C3D
{
    class Camera;
    class Shader;
}  // namespace C3D

struct GeometryDistance
{
    C3D::GeometryRenderData g;
    f32 distance;
};

struct RenderViewWorldData
{
    C3D::SkyboxPacketData skyboxData;

    C3D::DynamicArray<C3D::GeometryRenderData> worldGeometries;
    C3D::DynamicArray<C3D::GeometryRenderData> terrainGeometries;
    C3D::DynamicArray<C3D::GeometryRenderData> debugGeometries;
};

class RenderViewWorld final : public C3D::RenderView
{
public:
    RenderViewWorld();

    bool OnCreate() override;
    void OnDestroy() override;

    bool OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                       C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet) override;

private:
    bool OnEvent(u16 code, void* sender, const C3D::EventContext& context);

    void OnSetupPasses() override;

    C3D::DynamicArray<GeometryDistance, C3D::LinearAllocator> m_distances;

    C3D::Shader* m_materialShader = nullptr;
    C3D::Shader* m_terrainShader  = nullptr;
    C3D::Shader* m_debugShader    = nullptr;
    C3D::Shader* m_skyboxShader   = nullptr;

    C3D::RegisteredEventCallback m_onEventCallback;

    DebugColorShaderLocations m_debugShaderLocations;
    SkyboxShaderLocations m_skyboxShaderLocations;

    vec4 m_ambientColor;
    u32 m_renderMode = 0;
};
