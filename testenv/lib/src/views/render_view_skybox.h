
#pragma once
#include <core/events/event_context.h>
#include <renderer/camera.h>
#include <renderer/render_view.h>

namespace C3D
{
    class Shader;
}

class RenderViewSkybox final : public C3D::RenderView
{
public:
    RenderViewSkybox();

    bool OnCreate() override;

    void OnResize() override;

    bool OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                  u64 renderTargetIndex) override;

private:
    void OnSetupPasses() override;

    C3D::Shader* m_shader = nullptr;

    f32 m_fov      = C3D::DegToRad(45.0f);
    f32 m_nearClip = 0.1f;
    f32 m_farClip  = 1000.0f;

    mat4 m_projectionMatrix;

    C3D::Camera* m_camera = nullptr;

    u16 m_projectionLocation = INVALID_ID_U16;
    u16 m_viewLocation       = INVALID_ID_U16;
    u16 m_cubeMapLocation    = INVALID_ID_U16;
};
