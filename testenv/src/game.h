
#pragma once
#include <core/application.h>

namespace C3D
{
	class Camera;
}

class TestEnv : public C3D::Application
{
public:
	TestEnv(const C3D::ApplicationConfig& config);

	void OnCreate() override;

	void OnUpdate(f64 deltaTime) override;
	void OnRender(f64 deltaTime) override;

private:
	C3D::Camera* m_camera;
};
