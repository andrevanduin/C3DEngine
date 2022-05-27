
#pragma once
#include <core/application.h>

class TestApp : public C3D::Application
{
	TestApp(const C3D::ApplicationConfig& config);

	void OnCreate() override;
};