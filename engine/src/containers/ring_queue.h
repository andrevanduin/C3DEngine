
#pragma once
#include "core/defines.h"
#include "memory/global_memory_system.h"

namespace C3D
{
    template <class T>
    class RingQueue
    {
    public:
        RingQueue() : m_elements(nullptr), m_count(0), m_capacity(0), m_head(0), m_tail(-1) {}

        explicit RingQueue(const u64 initialCapacity) : m_elements(nullptr), m_count(0), m_capacity(0), m_head(0), m_tail(-1)
        {
            Create(initialCapacity);
        }

        RingQueue(const RingQueue&) = delete;
        RingQueue(RingQueue&&)      = delete;

        RingQueue& operator=(const RingQueue&) = delete;
        RingQueue& operator=(RingQueue&&)      = delete;

        ~RingQueue() { Destroy(); }

        void Create(u64 initialCapacity)
        {
            Logger::Trace("[RING_QUEUE] - Create({})", initialCapacity);

            if (m_elements)
            {
                // We already have elements so we need to clean those up first
                Destroy();
            }

            m_elements = Memory.Allocate<T>(MemoryType::RingQueue, initialCapacity);
            m_count    = 0;
            m_capacity = initialCapacity;
            m_head     = 0;
            m_tail     = -1;
        }

        void Destroy()
        {
            if (m_elements && m_capacity != 0)
            {
                // Destroy all created elements
                std::destroy_n(m_elements + m_head, m_count);

                Memory.Free(m_elements);
                m_elements = nullptr;
                m_count    = 0;
                m_capacity = 0;
                m_head     = 0;
                m_tail     = -1;
            }
        }

        void Enqueue(const T& element)
        {
            if (m_count == m_capacity)
            {
                Logger::Error("[RING_QUEUE] - Attempted to Enqueue an element but the RingQueue is full.");
                return;
            }

            // Increment the tail
            m_tail++;
            // Ensure that our tail wraps around to the front if we reach our capacity
            m_tail %= m_capacity;
            // Add the element to our array
            m_elements[m_tail] = element;
            // Increment the amount of elements in our array
            m_count++;
        }

        template <class... Args>
        T& Enqueue(Args&&... args)
        {
            if (m_count == m_capacity)
            {
                throw std::out_of_range("[RING_QUEUE] - Queue is full.");
            }

            // Increment the tail
            m_tail++;
            // Ensure that our tail wraps around to the front if we reach our capacity
            m_tail %= m_capacity;
            // Construct element T (in-place) with the provided arguments
            new (&m_elements[m_tail]) T(std::forward<Args>(args)...);
            // Increment the amount of elements in our array
            m_count++;

            // Return a reference to the newly added element
            return m_elements[m_tail];
        }

        T Dequeue()
        {
#ifdef _DEBUG
            assert(m_count > 0);
#endif

            // Get the element at the head
            auto element = m_elements[m_head];
            // Increment the head
            m_head++;
            // Ensure that our head wraps around to the front if we reach our capacity
            m_head %= m_capacity;
            // Decrease the size of our elem
            m_count--;
            // Return our element
            return element;
        }

        [[nodiscard]] T& Peek() const
        {
#ifdef _DEBUG
            assert(m_count > 0);
#endif
            // Return a reference to the element at the head
            return m_elements[m_head];
        }

        [[nodiscard]] T* GetData() const noexcept { return m_elements; }

        [[nodiscard]] u64 Size() const noexcept { return m_count; }
        [[nodiscard]] u64 Capacity() const noexcept { return m_capacity; }

        [[nodiscard]] bool Empty() const noexcept { return m_count == 0; }

    private:
        /* Pointer to the elements in this RingQueue. */
        T* m_elements;
        /* @brief The amount of elements currently contained in this RingQueue. */
        u64 m_count;
        /* @brief The amount of elements this RingQueue can store in total. */
        u64 m_capacity;
        /* @brief The index into the list where the head is currently. */
        u64 m_head;
        /* @brief The index into the list where the tail is currently. */
        u64 m_tail;
    };
}  // namespace C3D
