
#include "test_manager.h"
#include <core/logger.h>

#include "memory/linear_allocator_tests.h"
#include "containers/hash_table_tests.h"
#include "containers/dynamic_array_tests.h"

#include "core/memory.h"

#include "services/services.h"

int main(int argc, char** argv)
{
	constexpr C3D::MemorySystemConfig config{ MebiBytes(64) };

	C3D::Logger::Init();
	C3D::Services::InitMemory(config);

	TestManager manager;

	// TODO: add tests here
	LinearAllocator::RegisterTests(&manager);
	HashTable::RegisterTests(&manager);
	DynamicArray::RegisterTests(&manager);

	C3D::Logger::Debug("Starting tests...");
	manager.RunTests();

	return 0;
}
