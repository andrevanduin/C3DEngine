
#include <core/logger.h>

#include "containers/array_tests.h"
#include "containers/cstring_tests.h"
#include "containers/dynamic_array_tests.h"
#include "containers/hash_map_tests.h"
#include "containers/hash_table_tests.h"
#include "containers/ring_queue_tests.h"
#include "containers/stack_tests.h"
#include "containers/string_tests.h"
#include "core/stack_function_tests.h"
#include "memory/dynamic_allocator_tests.h"
#include "memory/linear_allocator_tests.h"
#include "memory/stack_allocator_tests.h"
#include "platform/file_system.h"
#include "test_manager.h"

int main(int argc, char** argv)
{
    // Run our tests with 32MiB
    TestManager manager(MebiBytes(32));

    LinearAllocator::RegisterTests(manager);
    DynamicAllocator::RegisterTests(manager);
    StackAllocator::RegisterTests(manager);

    StackFunction::RegisterTests(manager);

    Array::RegisterTests(manager);
    DynamicArray::RegisterTests(manager);

    Stack::RegisterTests(manager);

    String::RegisterTests(manager);
    CString::RegisterTests(manager);

    HashTable::RegisterTests(manager);
    HashMap::RegisterTests(manager);

    RingQueue::RegisterTests(manager);

    FileSystem::RegisterTests(manager);

    C3D::Logger::Debug("------ Starting tests... ------");
    manager.RunTests();
    C3D::Logger::Debug("----- Done Running tests -----");

    return 0;
}
