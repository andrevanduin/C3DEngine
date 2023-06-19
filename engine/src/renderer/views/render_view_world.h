
#pragma once
#include "core/events/event_context.h"
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
    class Camera;

    struct GeometryDistance
    {
        GeometryRenderData g;
        f32 distance;
    };

    class RenderViewWorld final : public RenderView
    {
    public:
        explicit RenderViewWorld(const RenderViewConfig& config);

        bool OnCreate() override;
        void OnDestroy() override;

        void OnResize() override;

        bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) override;

        bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) override;

    private:
        bool OnEvent(u16 code, void* sender, const EventContext& context);

        DynamicArray<GeometryDistance> m_distances;

        Shader* m_shader;

        f32 m_fov;
        f32 m_nearClip;
        f32 m_farClip;

        RegisteredEventCallback m_onEventCallback;

        mat4 m_projectionMatrix;
        Camera* m_camera;

        vec4 m_ambientColor;
        u32 m_renderMode;
    };
}  // namespace C3D
