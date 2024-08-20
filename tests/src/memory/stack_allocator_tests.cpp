
#include "stack_allocator_tests.h"

#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <core/random.h>
#include <memory/allocators/stack_allocator.h>

#include "../expect.h"

u8 StackAllocatorShouldCreate()
{
    C3D::StackAllocator<KibiBytes(8)> allocator;
    allocator.Create("Test Allocator");

    ExpectEqual(KibiBytes(8), allocator.GetTotalSize());

    return true;
}

u8 StackAllocatorShouldErrorOnOverAllocation()
{
    C3D::StackAllocator<KibiBytes(2)> allocator;
    allocator.Create("Test Allocator");

    ExpectThrow(std::bad_alloc, [&] { allocator.AllocateBlock(C3D::MemoryType::Test, KibiBytes(8)); });

    return true;
}

u8 StackAllocatorShouldWorkWithDynamicArray()
{
    C3D::StackAllocator<KibiBytes(8)> allocator;
    allocator.Create("Test Allocator");

    C3D::DynamicArray<int, C3D::StackAllocator<KibiBytes(8)>> array(&allocator);

    for (u64 i = 0; i < 32; i++)
    {
        const auto value = C3D::Random.Generate(0, 64);
        array.PushBack(value);
    }

    return true;
}

void StackAllocator::RegisterTests(TestManager& manager)
{
    manager.StartType("Stack Allocator");
    REGISTER_TEST(StackAllocatorShouldCreate, "Stack Allocator should correctly create and destroy");
    REGISTER_TEST(StackAllocatorShouldErrorOnOverAllocation, "Stack allocator should throw if you try to over allocate");
    REGISTER_TEST(StackAllocatorShouldWorkWithDynamicArray, "Stack allocator should be usable for a dynamic array");
}