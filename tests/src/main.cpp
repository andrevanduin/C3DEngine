
#include "test_manager.h"
#include <core/logger.h>

#include "memory/linear_allocator_tests.h"

int main(int argc, char** argv)
{
	C3D::Logger::Init();

	TestManager manager;

	// TODO: add tests here
	LinearAllocator::RegisterTests(&manager);

	C3D::Logger::Debug("Starting tests...");
	manager.RunTests();

	return 0;
}