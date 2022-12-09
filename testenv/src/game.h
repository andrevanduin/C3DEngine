
#pragma once
#include <core/application.h>

#include <resources/mesh.h>
#include "resources/ui_text.h"

namespace C3D
{
	class Camera;
}

class TestEnv final : public C3D::Application
{
public:
	explicit TestEnv(const C3D::ApplicationConfig& config);

	bool OnBoot() override;
	bool OnCreate() override;

	void OnUpdate(f64 deltaTime) override;
	bool OnRender(C3D::RenderPacket& packet, f64 deltaTime) override;

	void OnResize(u16 width, u16 height) override;

	void OnShutdown() override;

private:
	bool ConfigureRenderViews();

	bool OnEvent(u16 code, void* sender, C3D::EventContext context);
	bool OnDebugEvent(u16 code, void* sender, C3D::EventContext context);

	C3D::Camera* m_camera;

	u16 m_width, m_height;

	// TEMP
	C3D::Skybox m_skybox;

	C3D::Mesh m_meshes[10];
	C3D::Mesh* m_carMesh;
	C3D::Mesh* m_sponzaMesh;

	bool m_modelsLoaded;

	C3D::Mesh m_uiMeshes[10];
	C3D::UIText m_testText;

	u32 m_hoveredObjectId;
	// TEMP END
};
