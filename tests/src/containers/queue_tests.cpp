
#include "queue_tests.h"

#include <containers/queue.h>
#include <defines.h>
#include <metrics/metrics.h>

#include "../expect.h"

TEST(QueueShouldCreateAndClear)
{
    C3D::Queue<int> queue;

    // Queue should start with no elements allocated
    ExpectEqual(0, queue.Capacity());
    ExpectEqual(0, queue.Count());

    // Enqueue some items
    queue.Enqueue(1);
    queue.Enqueue(2);

    // The queue should have allocated it's default size
    ExpectEqual(4, queue.Capacity());
    // And have 2 items
    ExpectEqual(2, queue.Count());

    // The memory usage of the queue should be capacity * sizeof(element)
    ExpectEqual(sizeof(int) * 4, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Queue));

    // Clear the queue
    queue.Clear();

    // Capacity should remain the same but count should be 0
    ExpectEqual(4, queue.Capacity());
    ExpectEqual(0, queue.Count());

    // Destroy the queue
    queue.Destroy();

    // Now capacity and memory usage should also be 0
    ExpectEqual(0, queue.Capacity());
    ExpectEqual(0, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::Queue));
}

TEST(QueueShouldEnqueueAndPop)
{
    C3D::Queue<int> queue;

    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    ExpectEqual(5, queue.Count());

    int number = 1;
    while (!queue.Empty())
    {
        ExpectEqual(number, queue.Pop());
        number++;
    }
}

TEST(QueueShouldCopy)
{
    C3D::Queue<int> queue;

    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    auto queue2 = C3D::Queue<int>(queue);

    int number = 1;
    while (!queue2.Empty())
    {
        ExpectEqual(number, queue2.Pop());
        number++;
    }

    ExpectEqual(5, queue.Count());
    ExpectEqual(0, queue2.Count());
}

TEST(QueueShouldMove)
{
    C3D::Queue<int> queue;

    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    auto queue2 = C3D::Queue<int>(std::move(queue));

    ExpectEqual(0, queue.Count());

    int number = 1;
    while (!queue2.Empty())
    {
        ExpectEqual(number, queue2.Pop());
        number++;
    }

    ExpectEqual(0, queue2.Count());
}

TEST(QueueShouldInternallyWrap)
{
    C3D::Queue<int> queue;
    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    ExpectEqual(1, queue.Pop());
    ExpectEqual(2, queue.Pop());
    ExpectEqual(3, queue.Pop());

    queue.Enqueue(6);
    queue.Enqueue(7);
    queue.Enqueue(8);

    int number = 4;
    while (queue.Empty())
    {
        ExpectEqual(number, queue.Pop());
        number++;
    }
}

void Queue::RegisterTests(TestManager& manager)
{
    manager.StartType("Queue");

    REGISTER_TEST(QueueShouldCreateAndClear, "Queue should create and clear properly.");
    REGISTER_TEST(QueueShouldEnqueueAndPop, "Queue enqueue and pop items in the correct order.");
    REGISTER_TEST(QueueShouldCopy, "Queue should copy correctly.");
    REGISTER_TEST(QueueShouldMove, "Queue should move correctly.");
    REGISTER_TEST(QueueShouldInternallyWrap, "Queue wrapping around internally should work.");
}
