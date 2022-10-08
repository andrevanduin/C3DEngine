
#include "test_manager.h"
#include <core/logger.h>

#include "containers/array.h"
#include "containers/array_tests.h"
#include "memory/linear_allocator_tests.h"
#include "memory/dynamic_allocator_tests.h"
#include "containers/hash_table_tests.h"
#include "containers/dynamic_array_tests.h"
#include "containers/ring_queue_tests.h"
#include "containers/string_tests.h"

#include "core/memory.h"

#include "services/services.h"

int main(int argc, char** argv)
{
	constexpr C3D::MemorySystemConfig config{ MebiBytes(64), true };

	C3D::Logger::Init();
	C3D::Services::InitMemory(config);

	TestManager manager;

	Array::RegisterTests(&manager);
	HashTable::RegisterTests(&manager);
	RingQueue::RegisterTests(&manager);
	LinearAllocator::RegisterTests(&manager);
	DynamicAllocator::RegisterTests(&manager);
	DynamicArray::RegisterTests(&manager);
	String::RegisterTests(&manager);

	C3D::Logger::Debug("Starting tests...");
	manager.RunTests();

	return 0;
}
