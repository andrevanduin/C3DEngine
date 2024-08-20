
#include "ring_queue_tests.h"

#include <containers/ring_queue.h>
#include <core/defines.h>
#include <core/metrics/metrics.h>

#include "../expect.h"

u8 RingQueueShouldCreateAndDestroy()
{
    C3D::RingQueue<int> queue(10);

    ExpectEqual(10, queue.Capacity());
    ExpectEqual(0, queue.Size());
    ExpectNotEqual(nullptr, (void*)queue.GetData());
    ExpectEqual(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::RingQueue));

    queue.Destroy();

    ExpectEqual(0, queue.Capacity());
    ExpectEqual(0, queue.Size());
    ExpectEqual(nullptr, (void*)queue.GetData());
    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::RingQueue));

    return true;
}

u8 RingQueueShouldDestroyWhenLeavingScope()
{
    {
        const C3D::RingQueue<int> queue(10);

        ExpectEqual(10, queue.Capacity());
        ExpectEqual(0, queue.Size());
        ExpectNotEqual(nullptr, (void*)queue.GetData());
        ExpectEqual(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::RingQueue));
    }

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::RingQueue));
    return true;
}

void RingQueue::RegisterTests(TestManager& manager)
{
    manager.StartType("RingQueue");
    REGISTER_TEST(RingQueueShouldCreateAndDestroy, "RingQueue should internally allocate memory and destroy it properly on destroy");
    REGISTER_TEST(RingQueueShouldDestroyWhenLeavingScope, "RingQueue should internally deallocate memory when leaving the scope");
}
