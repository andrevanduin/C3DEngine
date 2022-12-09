
#include "test_manager.h"
#include <core/logger.h>

#include "containers/array_tests.h"
#include "memory/linear_allocator_tests.h"
#include "memory/dynamic_allocator_tests.h"
#include "containers/hash_table_tests.h"
#include "containers/hash_map_tests.h"
#include "containers/dynamic_array_tests.h"
#include "containers/ring_queue_tests.h"
#include "containers/string_tests.h"
#include "memory/global_memory_system.h"
#include "memory/stack_allocator_tests.h"

int main(int argc, char** argv)
{
	C3D::Logger::Init();
	Metrics.Init();
	C3D::GlobalMemorySystem::Init({ MebiBytes(64) });
	
	TestManager manager;

	LinearAllocator::RegisterTests(&manager);
	DynamicAllocator::RegisterTests(&manager);
	StackAllocator::RegisterTests(&manager);

	Array::RegisterTests(&manager);
	DynamicArray::RegisterTests(&manager);
	String::RegisterTests(&manager);

	HashTable::RegisterTests(&manager);
	HashMap::RegisterTests(&manager);
	RingQueue::RegisterTests(&manager);
	
	C3D::Logger::Debug("Starting tests...");
	manager.RunTests();

	C3D::GlobalMemorySystem::Destroy();
	return 0;
}
