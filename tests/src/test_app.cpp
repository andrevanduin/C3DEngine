
#include "test_app.h"

#include <services/services.h>
#include <core/logger.h>

#include "test_manager.h"
#include "memory/linear_allocator_tests.h"


TestApp::TestApp(const C3D::ApplicationConfig& config) : Application(config) {}

void TestApp::OnCreate()
{
	TestManager manager;
	manager.Init();

	// TODO: add tests here
	LinearAllocator::RegisterTests(&manager);

	C3D::Logger::Debug("Starting tests...");
	manager.RunTests();

	Quit();
}
