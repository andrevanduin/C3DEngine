
#include "stack_tests.h"
#include "../expect.h"

#include "containers/stack.h"


u8 StackShouldCreateEmptyWithDefaultCtor()
{
	const C3D::Stack<int> stack;

	ExpectShouldBe(0, stack.Capacity());
	ExpectShouldBe(0, stack.Size());

	return true;
}

u8 StackShouldCleanupWhenLeaveScope()
{
	{
		C3D::Stack<int> stack;
		stack.Push(1);
		stack.Push(2);
		stack.Push(3);
		stack.Push(10);

		ExpectShouldBe(4, stack.Capacity());
		ExpectShouldBe(4, stack.Size());
		ExpectShouldBe(10, stack.Pop())

		ExpectShouldNotBe(nullptr, (void*)stack.GetData());
		ExpectShouldBe(sizeof(int) * 4, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
	}

	ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::Stack));

	return true;
}

u8 StackShouldPop()
{
	C3D::Stack<int> stack;
	stack.Push(1);
	stack.Push(2);
	stack.Push(3);
	stack.Push(4);

	for (auto i = 4; i > 0; i--)
	{
		// Capacity should remain the same
		ExpectShouldBe(4, stack.Capacity());
		// Size should decrease by 1 each time
		ExpectShouldBe(i, stack.Size());
		// Numbers should be sequential in descending order
		ExpectShouldBe(i, stack.Pop());
	}
	
	return true;
}

u8 StackShouldClear()
{
	C3D::Stack<int> stack;
	stack.Push(1);
	stack.Push(2);
	stack.Push(3);
	stack.Push(4);

	ExpectShouldBe(4 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
	ExpectShouldBe(4, stack.Size());

	stack.Clear();

	ExpectShouldBe(4 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
	ExpectShouldBe(0, stack.Size());

	return true;
}

u8 StackShouldBeIterable()
{
	C3D::Stack<int> stack;
	stack.Push(1);
	stack.Push(2);
	stack.Push(3);
	stack.Push(4);

	auto num = 1;
	for (auto i : stack)
	{
		ExpectShouldBe(num, i);
		num++;
	}

	return true;
}

u8 StackShouldBeConstructableByInitializerList()
{
	C3D::Stack stack = { 1, 2, 3, 4 };

	auto num = 1;
	for (auto i : stack)
	{
		ExpectShouldBe(num, i);
		num++;
	}

	return true;
}

void Stack::RegisterTests(TestManager* manager)
{
	manager->StartType("Stack");
	manager->Register(StackShouldCreateEmptyWithDefaultCtor, "Stacks should be created without any capacity and size if empty constructor is used");
	manager->Register(StackShouldCleanupWhenLeaveScope, "Stacks should be cleanup up after leaving scope");
	manager->Register(StackShouldPop, "Stacks should pop elements from top to bottom");
	manager->Register(StackShouldClear, "Stacks should clear size to 0 and capacity should remain the same");
	manager->Register(StackShouldBeIterable, "Stacks should be iterable");
	manager->Register(StackShouldBeConstructableByInitializerList, "Stacks should be constructable by an initializer list");
}