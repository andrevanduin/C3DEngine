
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

    struct RenderViewWorldData
    {
        DynamicArray<GeometryRenderData> worldGeometries;
        DynamicArray<GeometryRenderData> terrainGeometries;
    };

    struct TerrainShaderLocations
    {
        u16 projection      = INVALID_ID_U16;
        u16 view            = INVALID_ID_U16;
        u16 ambientColor    = INVALID_ID_U16;
        u16 viewPosition    = INVALID_ID_U16;
        u16 shininess       = INVALID_ID_U16;
        u16 diffuseColor    = INVALID_ID_U16;
        u16 diffuseTexture  = INVALID_ID_U16;
        u16 specularTexture = INVALID_ID_U16;
        u16 normalTexture   = INVALID_ID_U16;
        u16 model           = INVALID_ID_U16;
        u16 renderMode      = INVALID_ID_U16;
        u16 dirLight        = INVALID_ID_U16;
        u16 pLights         = INVALID_ID_U16;
        u16 numPLights      = INVALID_ID_U16;
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

        f32 m_fov      = DegToRad(45.0f);
        f32 m_nearClip = 0.1f;
        f32 m_farClip  = 4000.0f;

        RegisteredEventCallback m_onEventCallback;

        mat4 m_projectionMatrix;
        Camera* m_camera = nullptr;

        TerrainShaderLocations m_terrainShaderLocations;

        vec4 m_ambientColor;
        u32 m_renderMode = 0;
    };
}  // namespace C3D
