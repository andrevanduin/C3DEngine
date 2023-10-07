
#pragma once
#include "core/events/event_context.h"
#include "renderer/render_view.h"

namespace C3D
{
    class Camera;
    class Shader;

    struct GeometryDistance
    {
        GeometryRenderData g;
        f32 distance;
    };

    struct RenderViewWorldData
    {
        DynamicArray<GeometryRenderData> worldGeometries;
        DynamicArray<GeometryRenderData> terrainGeometries;
        DynamicArray<GeometryRenderData> debugGeometries;
    };

    struct DebugColorShaderLocations
    {
        u16 projection;
        u16 view;
        u16 model;
    };

    class RenderViewWorld final : public RenderView
    {
    public:
        explicit RenderViewWorld(const RenderViewConfig& config);

        bool OnCreate() override;
        void OnDestroy() override;

        void OnResize() override;

        bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) override;

        bool OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                      u64 renderTargetIndex) override;

    private:
        bool OnEvent(u16 code, void* sender, const EventContext& context);

        DynamicArray<GeometryDistance, LinearAllocator> m_distances;

        Shader* m_materialShader = nullptr;
        Shader* m_terrainShader  = nullptr;
        Shader* m_debugShader    = nullptr;

        f32 m_fov      = DegToRad(45.0f);
        f32 m_nearClip = 0.1f;
        f32 m_farClip  = 4000.0f;

        RegisteredEventCallback m_onEventCallback;

        DebugColorShaderLocations m_debugShaderLocations;

        mat4 m_projectionMatrix;
        Camera* m_camera = nullptr;

        vec4 m_ambientColor;
        u32 m_renderMode = 0;
    };
}  // namespace C3D
