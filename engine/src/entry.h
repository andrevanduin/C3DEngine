
#pragma once
#include "core/application.h"
#include "core/memory.h"

int main(int argc, char** argv)
{
	C3D::Memory::Init();

	const auto app = C3D::CreateApplication();
	app->Run();

	C3D::DestroyApplication(app);
	C3D::Memory::Shutdown();
	return 0;
}