
#pragma once
#include <core/events/event_context.h>
#include <renderer/render_view.h>

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

struct DebugColorShaderLocations
{
    u16 projection;
    u16 view;
    u16 model;
};

class RenderViewWorld final : public C3D::RenderView
{
public:
    RenderViewWorld();

    bool OnCreate() override;
    void OnDestroy() override;

    void OnResize() override;

    bool OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                  u64 renderTargetIndex) override;

private:
    bool OnEvent(u16 code, void* sender, const C3D::EventContext& context);

    void OnSetupPasses() override;

    C3D::DynamicArray<GeometryDistance, C3D::LinearAllocator> m_distances;

    C3D::Shader* m_materialShader = nullptr;
    C3D::Shader* m_terrainShader  = nullptr;
    C3D::Shader* m_debugShader    = nullptr;

    f32 m_fov      = C3D::DegToRad(45.0f);
    f32 m_nearClip = 0.1f;
    f32 m_farClip  = 4000.0f;

    C3D::RegisteredEventCallback m_onEventCallback;

    DebugColorShaderLocations m_debugShaderLocations;

    mat4 m_projectionMatrix;
    C3D::Camera* m_camera = nullptr;

    vec4 m_ambientColor;
    u32 m_renderMode = 0;
};
