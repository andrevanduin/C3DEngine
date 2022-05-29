
#pragma once
#include <core/application.h>

#include "math/math_types.h"

class TestEnv : public C3D::Application
{
public:
	TestEnv(const C3D::ApplicationConfig& config);

	void OnCreate() override;

	void OnUpdate(f64 deltaTime) override;
	void OnRender(f64 deltaTime) override;

private:
	void RecalculateViewMatrix();

	void CameraPitch(f32 amount);
	void CameraYaw(f32 amount);

	mat4 m_view;

	vec3 m_cameraPosition;
	vec3 m_cameraEuler;

	bool m_cameraViewDirty;
};
