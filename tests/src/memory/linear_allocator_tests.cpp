
#include "linear_allocator_tests.h"

#include <core/defines.h>
#include <memory/allocators/linear_allocator.h>

#include "../expect.h"

u8 LinearAllocatorShouldCreateAndDestroy()
{
    C3D::LinearAllocator allocator;
    allocator.Create("LINEAR_ALLOCATOR_TEST", sizeof(u64), nullptr);

    ExpectShouldNotBe(nullptr, allocator.GetMemory());
    ExpectShouldBe(sizeof(u64), allocator.GetTotalSize());
    ExpectShouldBe(0, allocator.GetAllocated());

    allocator.Destroy();

    ExpectShouldBe(nullptr, allocator.GetMemory());
    ExpectShouldBe(0, allocator.GetTotalSize());
    ExpectShouldBe(0, allocator.GetAllocated());

    return true;
}

u8 LinearAllocatorSingleAllocationAllSpace()
{
    C3D::LinearAllocator allocator;
    allocator.Create("LINEAR_ALLOCATOR_TEST", sizeof(u64));

    // Single allocation.
    void* block = allocator.AllocateBlock(C3D::MemoryType::Test, sizeof(u64));

    // Validate it
    ExpectShouldNotBe(nullptr, block);
    ExpectShouldBe(sizeof(u64), allocator.GetAllocated());

    allocator.Destroy();
    return true;
}

u8 LinearAllocatorMultiAllocationAllSpace()
{
    constexpr u64 maxAllocations = 1024;

    C3D::LinearAllocator allocator;
    allocator.Create("LINEAR_ALLOCATOR_TEST", sizeof(u64) * maxAllocations);

    for (u64 i = 0; i < maxAllocations; i++)
    {
        void* block = allocator.AllocateBlock(C3D::MemoryType::Test, sizeof(u64));

        ExpectShouldNotBe(nullptr, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), allocator.GetAllocated());
    }

    allocator.Destroy();
    return true;
}

u8 LinearAllocatorMultiAllocationOverAllocate()
{
    constexpr u64 maxAllocations = 3;

    C3D::LinearAllocator allocator;
    allocator.Create("LINEAR_ALLOCATOR_TEST", sizeof(u64) * maxAllocations);

    for (u64 i = 0; i < maxAllocations; i++)
    {
        void* block = allocator.AllocateBlock(C3D::MemoryType::Test, sizeof(u64));

        ExpectShouldNotBe(nullptr, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), allocator.GetAllocated());
    }

    ExpectToThrow(std::bad_alloc, [&] { allocator.AllocateBlock(C3D::MemoryType::Test, sizeof(u64)); });

    allocator.Destroy();
    return true;
}

u8 LinearAllocatorMultiAllocationAllSpaceThenFree()
{
    constexpr u64 maxAllocations = 1024;

    C3D::LinearAllocator allocator;
    allocator.Create("LINEAR_ALLOCATOR_TEST", sizeof(u64) * maxAllocations);

    for (u64 i = 0; i < maxAllocations; i++)
    {
        void* block = allocator.AllocateBlock(C3D::MemoryType::Test, sizeof(u64));

        ExpectShouldNotBe(nullptr, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), allocator.GetAllocated());
    }

    allocator.FreeAll();
    ExpectShouldBe(0, allocator.GetAllocated());

    allocator.Destroy();

    return true;
}

void LinearAllocator::RegisterTests(TestManager& manager)
{
    manager.StartType("Linear Allocator");
    REGISTER_TEST(LinearAllocatorShouldCreateAndDestroy, "Linear Allocator should create and destroy");
    REGISTER_TEST(LinearAllocatorSingleAllocationAllSpace, "Linear Allocator single alloc for all space");
    REGISTER_TEST(LinearAllocatorMultiAllocationAllSpace, "Linear Allocator multi alloc for all space");
    REGISTER_TEST(LinearAllocatorMultiAllocationOverAllocate, "Linear Allocator try over allocate");
    REGISTER_TEST(LinearAllocatorMultiAllocationAllSpaceThenFree, "Linear Allocator allocated should be 0 after FreeAll()");
}
