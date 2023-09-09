
#pragma once
#include "core/events/event_context.h"
#include "renderer/camera.h"
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
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
        Shader* m_shader;

        f32 m_fov;
        f32 m_nearClip;
        f32 m_farClip;

        mat4 m_projectionMatrix;

        Camera* m_camera;

        u16 m_projectionLocation;
        u16 m_viewLocation;
        u16 m_cubeMapLocation;
    };
}  // namespace C3D
