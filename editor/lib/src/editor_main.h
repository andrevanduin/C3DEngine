
#pragma once
#include "editor_state.h"

extern "C" {
C3D_API C3D::ApplicationState* CreateApplicationState();
C3D_API C3D::Application* CreateApplication(C3D::ApplicationState* state);
}

class Editor final : public C3D::Application
{
public:
    explicit Editor(C3D::ApplicationState* state);

    bool OnBoot() override;
    bool OnRun(C3D::FrameData& frameData) override;

    void OnUpdate(C3D::FrameData& frameData) override;
    bool OnPrepareRenderPacket(C3D::RenderPacket& packet, C3D::FrameData& frameData) override;
    bool OnRender(C3D::RenderPacket& packet, C3D::FrameData& frameData) override;

    void OnResize() override;

    void OnShutdown() override;

    void OnLibraryLoad() override;
    void OnLibraryUnload() override;

private:
    bool ConfigureRenderViews() const;

    bool OnDebugEvent(u16 code, void* sender, const C3D::EventContext& context);
    bool OnButtonUp(u16 code, void* sender, const C3D::EventContext& context);
    bool OnMouseMoved(u16 code, void* sender, const C3D::EventContext& context);
    bool OnMouseDragged(u16 code, void* sender, const C3D::EventContext& context);

    bool LoadTestScene();
    void UnloadTestScene();

    EditorState* m_state = nullptr;
};
