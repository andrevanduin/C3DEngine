
#pragma once
#include "game_state.h"

extern "C" {
C3D_API C3D::ApplicationState* CreateApplicationState();
C3D_API C3D::Application* CreateApplication(C3D::ApplicationState* state);
}

class TestEnv final : public C3D::Application
{
public:
    explicit TestEnv(C3D::ApplicationState* state);

    bool OnBoot() override;
    bool OnRun(C3D::FrameData& frameData) override;

    void OnUpdate(C3D::FrameData& frameData) override;
    bool OnRender(C3D::RenderPacket& packet, C3D::FrameData& frameData) override;

    void OnResize() override;

    void OnShutdown() override;

    void OnLibraryLoad() override;
    void OnLibraryUnload() override;

private:
    bool ConfigureRenderViews() const;

    bool OnEvent(u16 code, void* sender, const C3D::EventContext& context) const;
    bool OnDebugEvent(u16 code, void* sender, const C3D::EventContext& context) const;

    GameState* m_state;
};
