
#include "ring_queue_tests.h"

#include <containers/ring_queue.h>
#include <core/defines.h>
#include <core/metrics/metrics.h>

#include "../expect.h"

u8 RingQueueShouldCreateAndDestroy()
{
    C3D::RingQueue<int> queue(10);

    ExpectShouldBe(10, queue.Capacity());
    ExpectShouldBe(0, queue.Size());
    ExpectShouldNotBe(nullptr, (void*)queue.GetData());
    ExpectShouldBe(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::RingQueue));

    queue.Destroy();

    ExpectShouldBe(0, queue.Capacity());
    ExpectShouldBe(0, queue.Size());
    ExpectShouldBe(nullptr, (void*)queue.GetData());
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::RingQueue));

    return true;
}

u8 RingQueueShouldDestroyWhenLeavingScope()
{
    {
        const C3D::RingQueue<int> queue(10);

        ExpectShouldBe(10, queue.Capacity());
        ExpectShouldBe(0, queue.Size());
        ExpectShouldNotBe(nullptr, (void*)queue.GetData());
        ExpectShouldBe(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::RingQueue));
    }

    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::RingQueue));
    return true;
}

void RingQueue::RegisterTests(TestManager& manager)
{
    manager.StartType("RingQueue");
    REGISTER_TEST(RingQueueShouldCreateAndDestroy, "RingQueue should internally allocate memory and destroy it properly on destroy");
    REGISTER_TEST(RingQueueShouldDestroyWhenLeavingScope, "RingQueue should internally deallocate memory when leaving the scope");
}
