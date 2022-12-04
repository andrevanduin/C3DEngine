
#include "stack_allocator_tests.h"
#include "../expect.h"

#include <core/defines.h>
#include <core/random.h>
#include <containers/dynamic_array.h>
#include <memory/allocators/stack_allocator.h>

u8 StackAllocatorShouldCreate()
{
	C3D::StackAllocator<KibiBytes(8)> allocator;
	allocator.Create("Test Allocator");

	ExpectShouldBe(KibiBytes(8), allocator.GetTotalSize());

	return true;
}

u8 StackAllocatorShouldErrorOnOverAllocation()
{
	C3D::StackAllocator<KibiBytes(2)> allocator;
	allocator.Create("Test Allocator");

	ExpectToThrow("bad allocation", [&] { allocator.AllocateBlock(C3D::MemoryType::Test, KibiBytes(6)); })

	return true;
}

u8 StackAllocatorShouldWorkWithDynamicArray()
{
	C3D::StackAllocator<KibiBytes(8)> allocator;
	allocator.Create("Test Allocator");

	C3D::DynamicArray<int, C3D::StackAllocator<KibiBytes(8)>> array(&allocator);

	auto random = C3D::Random();

	for (u64 i = 0; i < 32; i++)
	{
		const auto value = random.Generate<int>(0, 64);
		array.PushBack(value);
	}

	return true;
}

void StackAllocator::RegisterTests(TestManager* manager)
{
    manager->StartType("Stack Allocator");
	manager->Register(StackAllocatorShouldCreate, "Stack Allocator should correctly create and destroy");
	manager->Register(StackAllocatorShouldErrorOnOverAllocation, "Stack allocator should throw if you try to over allocate");
	manager->Register(StackAllocatorShouldWorkWithDynamicArray, "Stack allocator should be usable for a dynamic array");
}