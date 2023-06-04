
#pragma once
#include "core/console/console.h"
#include "core/engine.h"
#include "core/identifier.h"
#include "core/metrics/metrics.h"
#include "memory/global_memory_system.h"

inline int main(int argc, char** argv)
{
	// Create our console just so the logger can already have a handle to it
	C3D::UIConsole console;

	// Initialize our logger which we should do first to ensure we can log errors everywhere
	C3D::Logger::Init(&console);

	// Initialize our metrics to track our memory usage and other stats
	Metrics.Init();

	// Initialize our global allocator that we will normally always use
	C3D::GlobalMemorySystem::Init({ MebiBytes(512) });

	// Create our identifiers
	C3D::Identifier::Init();

	// Create our instance of the engine by calling the user supplied method
	const auto engine = C3D::CreateApplication();
	// Set the Base Console for our engine so the user can start using it
	engine->SetBaseConsole(&console);

	// Initialize our engine
	engine->Init();

	// Run our engine's game loop
	engine->Run();

	// Cleanup the main engine which was created by the user in the CreateApplication
	delete engine;

	// Destroy our identifiers
	C3D::Identifier::Destroy();

	// Cleanup our global memory system
	C3D::GlobalMemorySystem::Destroy();
	return 0;
}
