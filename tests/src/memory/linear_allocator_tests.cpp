
#include "linear_allocator_tests.h"
#include "../expect.h"

#include <memory/linear_allocator.h>
#include <core/defines.h>

u8 LinearAllocatorShouldCreateAndDestroy()
{
	C3D::LinearAllocator allocator;
	allocator.Create(sizeof(u64), nullptr);

	ExpectShouldNotBe(nullptr, allocator.GetMemory());
	ExpectShouldBe(sizeof(u64), allocator.GetTotalSize());
	ExpectShouldBe(0, allocator.GetAllocated());

	allocator.Destroy();

	ExpectShouldBe(nullptr, allocator.GetMemory());
	ExpectShouldBe(0, allocator.GetTotalSize());
	ExpectShouldBe(0, allocator.GetAllocated());

	return true;
}

u8 LinearAllocatorSingleAllocationAllSpace() {
    C3D::LinearAllocator allocator;
    allocator.Create(sizeof(u64));

    // Single allocation.
    void* block = allocator.Allocate(sizeof(u64));

    // Validate it
    ExpectShouldNotBe(nullptr, block);
    ExpectShouldBe(sizeof(u64), allocator.GetAllocated());

    allocator.Destroy();
    return true;
}

u8 LinearAllocatorMultiAllocationAllSpace() {
    constexpr u64 maxAllocations = 1024;

	C3D::LinearAllocator allocator;
    allocator.Create(sizeof(u64) * maxAllocations);

    for (u64 i = 0; i < maxAllocations; i++)
    {
        void* block = allocator.Allocate(sizeof(u64));

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
    allocator.Create(sizeof(u64) * maxAllocations);

    void* block;
    for (u64 i = 0; i < maxAllocations; i++)
    {
        block = allocator.Allocate(sizeof(u64));

        ExpectShouldNotBe(nullptr, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), allocator.GetAllocated());
    }

    C3D::Logger::Debug("The following error is intentionally caused by this test:");

    block = allocator.Allocate(sizeof(u64));

    ExpectShouldBe(nullptr, block);
    ExpectShouldBe(sizeof(u64) * maxAllocations, allocator.GetAllocated());

    allocator.Destroy();
    return true;
}

u8 LinearAllocatorMultiAllocationAllSpaceThenFree()
{
    constexpr u64 maxAllocations = 1024;

    C3D::LinearAllocator allocator;
    allocator.Create(sizeof(u64) * maxAllocations);

    for (u64 i = 0; i < maxAllocations; i++)
    {
        void* block = allocator.Allocate(sizeof(u64));

        ExpectShouldNotBe(nullptr, block);
        ExpectShouldBe(sizeof(u64) * (i + 1), allocator.GetAllocated());
    }

    allocator.FreeAll();
    ExpectShouldBe(0, allocator.GetAllocated());

    allocator.Destroy();

    return true;
}

void LinearAllocator::RegisterTests(TestManager* manager)
{
	manager->Register(LinearAllocatorShouldCreateAndDestroy, "Linear Allocator should create and destroy");
    manager->Register(LinearAllocatorSingleAllocationAllSpace, "Linear Allocator single alloc for all space");
    manager->Register(LinearAllocatorMultiAllocationAllSpace, "Linear Allocator multi alloc for all space");
    manager->Register(LinearAllocatorMultiAllocationOverAllocate, "Linear Allocator try over allocate");
    manager->Register(LinearAllocatorMultiAllocationAllSpaceThenFree, "Linear Allocator allocated should be 0 after FreeAll()");
}
