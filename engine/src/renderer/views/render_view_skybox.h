
#pragma once
#include "core/events/event_context.h"
#include "renderer/camera.h"
#include "renderer/render_view.h"

namespace C3D
{
    class Shader;

    class RenderViewSkybox final : public RenderView
    {
    public:
        explicit RenderViewSkybox(const RenderViewConfig& config);

        bool OnCreate() override;

        void OnResize() override;

        bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) override;

        bool OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                      u64 renderTargetIndex) override;

    private:
        Shader* m_shader = nullptr;

        f32 m_fov      = DegToRad(45.0f);
        f32 m_nearClip = 0.1f;
        f32 m_farClip  = 1000.0f;

        mat4 m_projectionMatrix;

        Camera* m_camera = nullptr;

        u16 m_projectionLocation = INVALID_ID_U16;
        u16 m_viewLocation       = INVALID_ID_U16;
        u16 m_cubeMapLocation    = INVALID_ID_U16;
    };
}  // namespace C3D
