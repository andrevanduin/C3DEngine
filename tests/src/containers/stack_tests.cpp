
#include "stack_tests.h"

#include <containers/stack.h>
#include <metrics/metrics.h>

#include "../expect.h"

TEST(StackShouldCreateEmptyWithDefaultCtor)
{
    const C3D::Stack<int> stack;

    ExpectEqual(0, stack.Capacity());
    ExpectEqual(0, stack.Size());
}

TEST(StackShouldCleanupWhenLeaveScope)
{
    {
        C3D::Stack<int> stack;
        stack.Push(1);
        stack.Push(2);
        stack.Push(3);
        stack.Push(10);

        ExpectEqual(4, stack.Capacity());
        ExpectEqual(4, stack.Size());
        ExpectEqual(10, stack.Pop())

            ExpectNotEqual(nullptr, (void*)stack.GetData());
        ExpectEqual(sizeof(int) * 4, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
    }

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::Stack));
}

TEST(StackShouldPop)
{
    C3D::Stack<int> stack;
    stack.Push(1);
    stack.Push(2);
    stack.Push(3);
    stack.Push(4);

    for (auto i = 4; i > 0; i--)
    {
        // Capacity should remain the same
        ExpectEqual(4, stack.Capacity());
        // Size should decrease by 1 each time
        ExpectEqual(i, stack.Size());
        // Numbers should be sequential in descending order
        ExpectEqual(i, stack.Pop());
    }
}

TEST(StackShouldClear)
{
    C3D::Stack<int> stack;
    stack.Push(1);
    stack.Push(2);
    stack.Push(3);
    stack.Push(4);

    ExpectEqual(4 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
    ExpectEqual(4, stack.Size());

    stack.Clear();

    ExpectEqual(4 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Stack));
    ExpectEqual(0, stack.Size());
}

TEST(StackShouldBeIterable)
{
    C3D::Stack<int> stack;
    stack.Push(1);
    stack.Push(2);
    stack.Push(3);
    stack.Push(4);

    auto num = 1;
    for (auto i : stack)
    {
        ExpectEqual(num, i);
        num++;
    }
}

TEST(StackShouldBeConstructableByInitializerList)
{
    C3D::Stack stack = { 1, 2, 3, 4 };

    auto num = 1;
    for (auto i : stack)
    {
        ExpectEqual(num, i);
        num++;
    }
}

void Stack::RegisterTests(TestManager& manager)
{
    manager.StartType("Stack");
    REGISTER_TEST(StackShouldCreateEmptyWithDefaultCtor,
                  "Stacks should be created without any capacity and size if empty constructor is used");
    REGISTER_TEST(StackShouldCleanupWhenLeaveScope, "Stacks should be cleanup up after leaving scope");
    REGISTER_TEST(StackShouldPop, "Stacks should pop elements from top to bottom");
    REGISTER_TEST(StackShouldClear, "Stacks should clear size to 0 and capacity should remain the same");
    REGISTER_TEST(StackShouldBeIterable, "Stacks should be iterable");
    REGISTER_TEST(StackShouldBeConstructableByInitializerList, "Stacks should be constructable by an initializer list");
}