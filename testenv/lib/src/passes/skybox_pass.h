
#pragma once
#include <core/defines.h>
#include <renderer/rendergraph/renderpass.h>
#include <systems/events/event_system.h>

#include "test_env_types.h"

namespace C3D
{
    class Skybox;
    class Shader;
    class Viewport;
    class Camera;
}  // namespace C3D

class SkyboxPass : public C3D::Renderpass
{
public:
    SkyboxPass();

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::Skybox* skybox);
    bool Execute(const C3D::FrameData& frameData) override;

private:
    C3D::Skybox* m_skybox = nullptr;
    C3D::Shader* m_shader = nullptr;

    C3D::RegisteredEventCallback m_onEventCallback;

    SkyboxShaderLocations m_locations;

    vec4 m_ambientColor;
    u32 m_renderMode = 0;
};