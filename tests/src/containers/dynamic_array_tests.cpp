
#include <iostream>

#include "dynamic_array_tests.h"
#include "../expect.h"

#include <containers/dynamic_array.h>
#include <core/defines.h>

#include "renderer/vertex.h"

u8 DynamicArrayShouldCreateAndDestroy()
{
	C3D::DynamicArray<int> array(10);

	ExpectShouldBe(10, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldNotBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(sizeof(int) * 10, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldReserve()
{
	C3D::DynamicArray<int> array;
	array.Reserve(10);

	ExpectShouldBe(10, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldNotBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(sizeof(int) * 10, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldReserveWithElementsPresent()
{
	C3D::DynamicArray<int> array(4);

	array.PushBack(1);
	array.PushBack(2);

	array.Reserve(12);

	ExpectShouldBe(12, array.Capacity());
	ExpectShouldBe(2, array.Size());
	ExpectShouldBe(sizeof(int) * 12, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	ExpectShouldBe(1, array[0]);
	ExpectShouldBe(2, array[1]);

	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldResize()
{
	C3D::DynamicArray<int> array;
	array.Resize(10);

	ExpectShouldBe(10, array.Capacity());
	ExpectShouldBe(10, array.Size());
	ExpectShouldBe(sizeof(int) * 10, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	for (int i = 0; i < 10; i++)
	{
		ExpectShouldBe(0, array[i]);
	}

	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldAllocateLargeBlocks()
{
	C3D::DynamicArray<C3D::Vertex3D> array(32768);

	ExpectShouldBe(32768, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(sizeof(C3D::Vertex3D) * 32768, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	constexpr auto element = C3D::Vertex3D{ vec3(0), vec3(0), vec2(1), vec4(1), vec4(4) };
	array.PushBack(element);

	ExpectShouldBe(element.position, array[0].position);
	ExpectShouldBe(element.normal, array[0].normal);
	ExpectShouldBe(element.texture, array[0].texture);
	ExpectShouldBe(element.color, array[0].color);
	ExpectShouldBe(element.tangent, array[0].tangent);

	array.Destroy();
	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldAllocateLargeBlocksAndCopyOverElementsOnResize()
{
	C3D::DynamicArray<C3D::Vertex3D> array(4);

	ExpectShouldBe(4, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(sizeof(C3D::Vertex3D) * 4, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	constexpr auto element = C3D::Vertex3D{ vec3(0), vec3(0), vec2(1), vec4(1), vec4(4) };
	array.PushBack(element);

	ExpectShouldBe(element.position, array[0].position);
	ExpectShouldBe(element.normal, array[0].normal);
	ExpectShouldBe(element.texture, array[0].texture);
	ExpectShouldBe(element.color, array[0].color);
	ExpectShouldBe(element.tangent, array[0].tangent);

	array.Reserve(32768);

	ExpectShouldBe(32768, array.Capacity());
	ExpectShouldBe(1, array.Size());
	ExpectShouldBe(sizeof(C3D::Vertex3D) * 32768, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	ExpectShouldBe(element.position, array[0].position);
	ExpectShouldBe(element.normal, array[0].normal);
	ExpectShouldBe(element.texture, array[0].texture);
	ExpectShouldBe(element.color, array[0].color);
	ExpectShouldBe(element.tangent, array[0].tangent);

	array.Destroy();

	ExpectShouldBe(0, array.Capacity());
	ExpectShouldBe(0, array.Size());
	ExpectShouldBe(nullptr, (void*)array.GetData());
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	return true;
}

u8 DynamicArrayShouldIterate()
{
	C3D::DynamicArray<int> array;
	array.PushBack(5);
	array.PushBack(6);
	array.PushBack(2);

	int counter = 0;
	for (auto& item : array)
	{
		counter++;
	}

	ExpectShouldBe(array.Size(), counter);
	return true;
}

u8 DynamicArrayShouldDestroyWhenLeavingScope()
{
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));

	{
		C3D::DynamicArray<std::string> array;
		array.PushBack("Test");
		array.PushBack("Test2");
	}

	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicArray));
	return true;
}

struct TestElement
{
	TestElement(int* c)
	{
		pCounter = c;
		*pCounter += 1;
	}

	~TestElement()
	{
		*pCounter -= 1;
	}

	int* pCounter;
};

u8 DynamicArrayShouldCallDestructorsOfElementsWhenDestroyed()
{
	auto counter = 0;

	C3D::DynamicArray<TestElement> array;
	array.EmplaceBack(&counter);
	array.EmplaceBack(&counter);

	ExpectShouldBe(2, counter);

	array.Destroy();

	ExpectShouldBe(0, counter);
	return true;
}

u8 DynamicArrayShouldCallDestructorsOfElementsWhenGoingOutOfScope()
{
	auto counter = 0;

	{
		C3D::DynamicArray<TestElement> array;
		array.EmplaceBack(&counter);
		array.EmplaceBack(&counter);

		ExpectShouldBe(2, counter);
	}

	ExpectShouldBe(0, counter);
	return true;
}

void DynamicArray::RegisterTests(TestManager* manager)
{
	manager->StartType("DynamicArray");
	manager->Register(DynamicArrayShouldCreateAndDestroy, "Dynamic array should internally allocate memory and destroy it properly on destroy");
	manager->Register(DynamicArrayShouldReserve, "Dynamic array should internally allocate enough space after Reserve() call");
	manager->Register(DynamicArrayShouldReserveWithElementsPresent, "Dynamic array should internally allocate enough space after Reserve() call");
	manager->Register(DynamicArrayShouldResize, "Dynamic array should Resize() and allocate enough memory with default objects");
	manager->Register(DynamicArrayShouldAllocateLargeBlocks, "Dynamic array should also work when allocating lots of storage for complex structs");
	manager->Register(DynamicArrayShouldAllocateLargeBlocksAndCopyOverElementsOnResize, "Dynamic array should also work when allocating lots of storage for complex structs after at least 1 element is added");
	manager->Register(DynamicArrayShouldIterate, "Dynamic array should iterate over all it's elements in a foreach loop");
	manager->Register(DynamicArrayShouldDestroyWhenLeavingScope, "Dynamic array should be automatically destroyed and cleaned up after leaving scope");
	manager->Register(DynamicArrayShouldCallDestructorsOfElementsWhenDestroyed, "Dynamic array should automatically call the destructor of every element when it is destroyed");
	manager->Register(DynamicArrayShouldCallDestructorsOfElementsWhenGoingOutOfScope, "Dynamic array should automatically call the destructor of every element when it goes out of scope");
}
