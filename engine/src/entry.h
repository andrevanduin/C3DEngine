
#pragma once
#include "core/application.h"
#include "core/identifier.h"
#include "memory/global_memory_system.h"

int main(int argc, char** argv)
{
	// Initialize our logger which we should do first to ensure we can log errors everywhere
	C3D::Logger::Init();

	// Initialize our metrics to track our memory usage and other stats
	Metrics.Init();

	// Initialize our global allocator that we will normally always use
	C3D::GlobalMemorySystem::Init({ MebiBytes(512) });

	// Create our application (game) by calling the user supplied method
	const auto app = C3D::CreateApplication();

	// Initialize our application
	app->Init();

	// Run our application's game loop
	app->Run();

	// Cleanup the main application which was created by the user in the CreateApplication
	delete app;

	// Destroy our identifiers
	C3D::Identifier::Destroy();

	// Cleanup our global memory system
	C3D::GlobalMemorySystem::Destroy();
	return 0;
}
