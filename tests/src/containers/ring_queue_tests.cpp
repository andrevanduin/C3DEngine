
#include "ring_queue_tests.h"

#include <containers/ring_queue.h>
#include <defines.h>
#include <metrics/metrics.h>

#include "../expect.h"

TEST(RingQueueShouldCreateAndClear)
{
    C3D::RingQueue<int, 10> queue;

    ExpectEqual(10, queue.Capacity());
    ExpectEqual(0, queue.Count());
    // The memory usage (on the stack) of the queue should be capacity * sizeof(element) + 3 * sizeof(u64)
    ExpectEqual(sizeof(int) * 10 + 3 * sizeof(u64), sizeof(C3D::RingQueue<int, 10>));

    queue.Clear();

    ExpectEqual(10, queue.Capacity());
    ExpectEqual(0, queue.Count());
}

TEST(RingQueueShouldEnqueueAndPop)
{
    C3D::RingQueue<int, 10> queue;

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

TEST(RingQueueShouldCopy)
{
    C3D::RingQueue<int, 10> queue;

    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    auto queue2 = C3D::RingQueue<int, 10>(queue);

    int number = 1;
    while (!queue2.Empty())
    {
        ExpectEqual(number, queue2.Pop());
        number++;
    }

    ExpectEqual(5, queue.Count());
    ExpectEqual(0, queue2.Count());
}

TEST(RingQueueShouldMove)
{
    C3D::RingQueue<int, 10> queue;

    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    queue.Enqueue(4);
    queue.Enqueue(5);

    auto queue2 = C3D::RingQueue<int, 10>(std::move(queue));

    ExpectEqual(0, queue.Count());

    int number = 1;
    while (!queue2.Empty())
    {
        ExpectEqual(number, queue2.Pop());
        number++;
    }

    ExpectEqual(0, queue2.Count());
}

TEST(RingQueueShouldInternallyWrap)
{
    C3D::RingQueue<int, 5> queue;
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

void RingQueue::RegisterTests(TestManager& manager)
{
    manager.StartType("RingQueue");

    REGISTER_TEST(RingQueueShouldCreateAndClear, "RingQueue should create and clear properly.");
    REGISTER_TEST(RingQueueShouldEnqueueAndPop, "RingQueue enqueue and pop items in the correct order.");
    REGISTER_TEST(RingQueueShouldCopy, "RingQueue should copy correctly.");
    REGISTER_TEST(RingQueueShouldMove, "RingQueue should move correctly.");
    REGISTER_TEST(RingQueueShouldInternallyWrap, "RingQueue wrapping around internally should work.");
}
