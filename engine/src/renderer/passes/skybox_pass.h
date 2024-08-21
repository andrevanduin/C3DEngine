
#pragma once
#include <core/defines.h>
#include <renderer/rendergraph/renderpass.h>
#include <systems/events/event_system.h>

namespace C3D
{
    class Skybox;
    class Shader;
    class Viewport;
    class Camera;

    struct SkyboxShaderLocations
    {
        u16 projection;
        u16 view;
        u16 cubeMap;
    };

    class C3D_API SkyboxPass : public C3D::Renderpass
    {
    public:
        SkyboxPass();

        bool Initialize(const LinearAllocator* frameAllocator) override;
        bool Prepare(const Viewport& viewport, Camera* camera, Skybox* skybox);
        bool Execute(const FrameData& frameData) override;

    private:
        Skybox* m_skybox = nullptr;
        Shader* m_shader = nullptr;

        RegisteredEventCallback m_onEventCallback;

        SkyboxShaderLocations m_locations;

        vec4 m_ambientColor;
        u32 m_renderMode = 0;
    };

}  // namespace C3D