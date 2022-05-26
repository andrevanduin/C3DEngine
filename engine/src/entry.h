
#pragma once
#include "core/application.h"
#include "core/memory.h"
#include "core/logger.h"

int main(int argc, char** argv)
{
	C3D::Memory::Init();
	C3D::Logger::Init();

	const auto app = C3D::CreateApplication();
	app->Run();
	delete app;

	C3D::Memory::Shutdown();
	return 0;
}