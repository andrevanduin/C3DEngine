
#pragma once
#include <core/engine.h>
#include <core/console.h>

#include <resources/mesh.h>

#include "math/frustum.h"
#include "renderer/primitives/primitive_renderer.h"
#include "resources/ui_text.h"

namespace C3D
{
	class Camera;
}

class TestEnv final : public C3D::Engine
{
public:
	explicit TestEnv(const C3D::ApplicationConfig& config);

	bool OnBoot() override;
	bool OnCreate() override;

	void OnUpdate(f64 deltaTime) override;
	bool OnRender(C3D::RenderPacket& packet, f64 deltaTime) override;
	void AfterRender() override;

	void OnResize(u16 width, u16 height) override;

	void OnShutdown() override;

private:
	bool ConfigureRenderViews();

	bool OnEvent(u16 code, void* sender, C3D::EventContext context);
	bool OnDebugEvent(u16 code, void* sender, C3D::EventContext context);

	C3D::Camera* m_camera;
	C3D::Camera* m_testCamera;

	C3D::Frustum m_cameraFrustum;

	C3D::PrimitiveRenderer m_primitiveRenderer;

	// TEMP
	C3D::Skybox m_skybox;

	C3D::Mesh m_meshes[10];
	C3D::Mesh* m_carMesh;
	C3D::Mesh* m_sponzaMesh;

	C3D::Mesh* m_primitiveMeshes[500];
	u32 m_primitiveMeshCount = 0;
	bool m_hasPrimitives[10]{};

	bool m_modelsLoaded;

	C3D::Mesh* m_planes[6];

	C3D::Mesh m_uiMeshes[10];
	C3D::UIText m_testText;

	u32 m_hoveredObjectId;
	// TEMP END
};
