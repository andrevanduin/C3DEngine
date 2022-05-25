
#pragma once
#include <core/application.h>

class TestEnv : public C3D::Application
{
public:
	TestEnv(const C3D::ApplicationConfig& config);

	void OnUpdate(f64 deltaTime) override;
	void OnRender(f64 deltaTime) override;
};