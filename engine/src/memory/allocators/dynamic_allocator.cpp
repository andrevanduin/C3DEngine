
#include "dynamic_allocator.h"

#include "core/logger.h"
#include "core/metrics/metrics.h"
#include "platform/platform.h"
#include "systems/system_manager.h"

namespace C3D
{
    DynamicAllocator::DynamicAllocator(const AllocatorType type)
        : BaseAllocator(ToUnderlying(type)),
          m_logger("DYNAMIC_ALLOCATOR"),
          m_initialized(false),
          m_totalSize(0),
          m_memorySize(0),
          m_memory(nullptr)
    {}

    bool DynamicAllocator::Create(void* memory, const u64 totalMemory, const u64 usableMemory)
    {
        if (totalMemory == 0)
        {
            m_logger.Error("Create() - Size cannot be 0. Creation failed");
            return false;
        }

        const u64 freeListMemoryRequirement = totalMemory - usableMemory;

        m_totalSize = totalMemory;
        m_memorySize = usableMemory;

        // The first part of our memory will be used by our freelist
        m_freeList.Create(memory, freeListMemoryRequirement, SMALLEST_POSSIBLE_ALLOCATION, usableMemory);

        // The second part of the memory will store the actual data that this allocator manages
        m_memory = static_cast<char*>(memory) + freeListMemoryRequirement;

        m_logger.Trace(
            "Create() - Successfully created DynamicAllocator managing {} bytes. Total memory usage = ({} + {} = {}) "
            "(UsableMemory + FreeListMemory = total)",
            usableMemory, usableMemory, freeListMemoryRequirement, totalMemory);

        // Create a metrics object to track the allocations this allocator does
        m_id = Metrics.CreateAllocator("DYNAMIC_ALLOCATOR", static_cast<AllocatorType>(m_type), usableMemory);

        m_initialized = true;
        return true;
    }

    bool DynamicAllocator::Destroy()
    {
        m_freeList.Destroy();

        m_totalSize = 0;
        m_memory = nullptr;

        // Destroy the metrics object associated with this
        Metrics.DestroyAllocator(m_id);
        return true;
    }

    void* DynamicAllocator::AllocateBlock(const MemoryType type, u64 size, const u16 alignment)
    {
        std::lock_guard allocGuard(m_mutex);

        if (size == 0 || alignment == 0)
        {
            m_logger.Error("Allocate() requires a valid size and alignment");
            return nullptr;
        }

        /* NOTE: Our total required size for an allocation is as made up of the following:
         *  - The user's requested size
         *	- The alignment required for the requested size
         *	- The size of the Alloc header
         *	- A marker to hold the size for quick and easy lookups`
         *
         */
        u64 requiredSize = alignment + sizeof(AllocHeader) + sizeof(AllocSizeMarker) + size;
        // Don't perform allocations of more than 4 GibiBytes at the time.
        assert(requiredSize < MAX_SINGLE_ALLOC_SIZE);

        u64 baseOffset = 0;
        if (m_freeList.AllocateBlock(requiredSize, &baseOffset))
        {
            /*
             * Our memory layout is as follows:
             * x  bytes - padding (alignment)
             * 4  bytes - size of the user's block
             * x  bytes - user's memory block
             * 16 bytes - AllocHeader
             */

            // Get the base pointer to the start of our unaligned memory block
            char* basePtr = m_memory + baseOffset;
            // Get our alignment right after our 4 bytes for our ALLOC_SIZE_MARKER. This way we can always store our u32
            // for the size of the user's allocation right before the user block, while still having our user's data
            // aligned.
            const u64 alignedBlockOffset =
                GetAligned(reinterpret_cast<u64>(basePtr + sizeof(AllocSizeMarker)), alignment);
            // Get the delta between our basePtr and alignedBlockOffset
            const u64 alignDelta = alignedBlockOffset - reinterpret_cast<u64>(basePtr);
            // Get the aligned user data pointer by adding the alignDelta
            char* userDataPtr = basePtr + alignDelta;
            // Store the size right before the user's data
            const auto sizePtr = reinterpret_cast<u32*>(userDataPtr - sizeof(AllocSizeMarker));
            *sizePtr = static_cast<u32>(size);
            // Store the header immediately after the user block
            const auto header = reinterpret_cast<AllocHeader*>(userDataPtr + size);
            header->start = basePtr;
            header->alignment = alignment;

#ifdef C3D_TRACE_ALLOCS
            m_logger.Trace("Allocated (size: {}, alignment {}, header: {} and marker: {} = {}) bytes at {}", size,
                           alignment, sizeof(AllocHeader), ALLOC_SIZE_MARKER_SIZE, requiredSize, fmt::ptr(basePtr));
#endif

            MetricsAllocate(m_id, type, size, requiredSize, userDataPtr);

            std::memset(userDataPtr, 0, size);
            return userDataPtr;
        }

        const auto available = m_freeList.FreeSpace();
        m_logger.Error("AllocateAligned() - No blocks of memory large enough to allocate from.");
        m_logger.Error("Requested size: {}, total space available: {}", size, available);

        throw std::bad_alloc();
    }

    void DynamicAllocator::Free(const MemoryType type, void* block)
    {
        std::lock_guard freeGuard(m_mutex);

        if (!block)
        {
            m_logger.Fatal("FreeAligned() - Called with nullptr block.");
        }

        if (m_memory == nullptr || m_totalSize == 0)
        {
            // Tried to free something from this allocator while it is not managing any memory
            m_logger.Fatal("Free() - Called while dynamic allocator is not managing memory.");
        }

        if (block < m_memory || block > m_memory + m_totalSize)
        {
            void* endOfBlock = m_memory + m_totalSize;
            m_logger.Fatal("Free() - Called with block ({}) outside of allocator range ({}) - ({}).", fmt::ptr(block),
                           fmt::ptr(m_memory), fmt::ptr(endOfBlock));
        }

        // The provided address points to the user's data block
        const auto userDataPtr = static_cast<char*>(block);
        // If we subtract the size of our ALLOC_SIZE_MARKER we get our size
        const auto blockSize = reinterpret_cast<u32*>(userDataPtr - sizeof(AllocSizeMarker));
        // If we add the size of the user's data to our user data block ptr we get our alloc header
        const auto header = reinterpret_cast<AllocHeader*>(userDataPtr + *blockSize);
        // We can now calculate the total size of our entire data combined
        const u64 requiredSize = header->alignment + sizeof(AllocHeader) + sizeof(AllocSizeMarker) + *blockSize;
        // From our header we can find the pointer to the start of our current block and subtract the start of our
        // managed memory block we get an offset into our managed block of memory
        const u64 offset = static_cast<char*>(header->start) - m_memory;
        // Then we simply free this memory
        if (!m_freeList.FreeBlock(requiredSize, offset))
        {
            m_logger.Error("Free() - failed");
        }

#ifdef C3D_TRACE_ALLOCS
        m_logger.Trace("FreeAligned() - Freed {} bytes at {}.", requiredSize, fmt::ptr(header->start));
#endif

        MetricsFree(m_id, type, *blockSize, requiredSize, userDataPtr);
    }

    bool DynamicAllocator::GetSizeAlignment(void* block, u64* outSize, u16* outAlignment)
    {
        // Get the size
        const auto userDataPtr = static_cast<char*>(block);
        const auto sizePtr = reinterpret_cast<u32*>(userDataPtr - sizeof(AllocSizeMarker));
        const auto userDataSize = *sizePtr;

        // Get the header
        const auto header = reinterpret_cast<AllocHeader*>(userDataPtr + userDataSize);

        *outSize = userDataSize;
        *outAlignment = header->alignment;
        return true;
    }

    bool DynamicAllocator::GetAlignment(void* block, u16* outAlignment)
    {
        // Get the size
        const auto userDataPtr = static_cast<char*>(block);
        const auto sizePtr = reinterpret_cast<u32*>(userDataPtr - sizeof(AllocSizeMarker));
        const auto userDataSize = *sizePtr;

        // Get the header
        const auto header = reinterpret_cast<AllocHeader*>(userDataPtr + userDataSize);

        *outAlignment = header->alignment;
        return true;
    }

    bool DynamicAllocator::GetAlignment(const void* block, u16* outAlignment)
    {
        // Get the size
        const auto userDataPtr = static_cast<const char*>(block);
        const auto sizePtr = reinterpret_cast<const u32*>(userDataPtr - sizeof(AllocSizeMarker));
        const auto userDataSize = *sizePtr;

        // Get the header
        const auto header = reinterpret_cast<const AllocHeader*>(userDataPtr + userDataSize);

        *outAlignment = header->alignment;
        return true;
    }

    u64 DynamicAllocator::FreeSpace() const { return m_freeList.FreeSpace(); }

    u64 DynamicAllocator::GetTotalUsableSize() const { return m_totalSize; }

    DynamicAllocator* DynamicAllocator::GetDefault()
    {
        static auto allocator = new DynamicAllocator(AllocatorType::GlobalDynamic);
        return allocator;
    }
}  // namespace C3D
