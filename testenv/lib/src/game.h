
#pragma once
#include <core/engine.h>
#include <core/application.h>

#include <math/frustum.h>
#include <renderer/camera.h>
#include <resources/mesh.h>

extern "C"
{
	C3D_API C3D::Application* CreateApplication(C3D::ApplicationState* state);
	C3D_API C3D::ApplicationState* CreateApplicationState();
}

struct GameState final : C3D::ApplicationState
{
	C3D::LinearAllocator frameAllocator;
	C3D::GameFrameData frameData;

	C3D::Camera* camera = nullptr;
	C3D::Camera* testCamera = nullptr;

	C3D::Frustum cameraFrustum;

	// TEMP
	C3D::Skybox skybox;

	C3D::Mesh meshes[10];
	C3D::Mesh* carMesh = nullptr;
	C3D::Mesh* sponzaMesh = nullptr;

	bool modelsLoaded = false;

	C3D::Mesh* planes[6] = {};

	C3D::Mesh uiMeshes[10];
	C3D::UIText testText;

	u32 hoveredObjectId = INVALID_ID;
	// TEMP
};

class TestEnv final : public C3D::Application
{
public:
	explicit TestEnv(C3D::ApplicationState* state);

	bool OnBoot() override;
	bool OnRun() override;

	void OnUpdate(f64 deltaTime) override;
	bool OnRender(C3D::RenderPacket& packet, f64 deltaTime) override;

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
