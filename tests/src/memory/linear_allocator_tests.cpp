
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

void LinearAllocator::RegisterTests(TestManager* manager)
{
	manager->Register(LinearAllocatorShouldCreateAndDestroy, "Linear Allocator should create and destroy");
}
