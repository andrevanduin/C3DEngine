
#include "dynamic_allocator_tests.h"

#include <core/defines.h>
#include <core/random.h>
#include <memory/allocators/dynamic_allocator.h>
#include <memory/global_memory_system.h>

#include <algorithm>
#include <array>

#include "../expect.h"
#include "core/metrics/metrics.h"

struct AllocStruct
{
    AllocStruct() : dataPtr(nullptr), data(0), size(0), alignment(0) {}

    char* dataPtr;
    char data;

    u64 size;
    u16 alignment;
};

TEST(DynamicAllocatorShouldCreateAndDestroy)
{
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectEqual(neededMemory, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicAllocator));

    allocator.Destroy();

    Memory.Free(memoryBlock);

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicAllocator));
}

constexpr u16 POSSIBLE_ALIGNMENTS[3] = { 1, 4, 8 };

template <u64 Size>
void MakeAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator)
{
    for (auto& allocation : data)
    {
        // Skip if allocation is already made for this index
        if (allocation.size != 0) continue;

        // Generate an index in our possible alignments array
        const auto alignmentIndex = C3D::Random.Generate<u64>(0, 2);
        const auto alignment      = POSSIBLE_ALIGNMENTS[alignmentIndex];

        // Generate a random size for our allocation between 4 bytes and 4 KibiBytes
        auto allocSize = C3D::Random.Generate<u64>(4, KibiBytes(4));

        // Ensure our randomly generator allocSize is divisible by our alignment
        while (allocSize % alignment != 0)
        {
            allocSize = C3D::Random.Generate<u64>(4, KibiBytes(4));
        }

        // Keep track of the pointer to our data so we can check it later
        allocation.dataPtr = static_cast<char*>(allocator.AllocateBlock(C3D::MemoryType::Test, allocSize, alignment));
        // Also keep track of our alignment so we can verify it later on
        allocation.alignment = alignment;

        // Keep track of the size so we know how large the allocation should be
        allocation.size = allocSize;

        // Generate a random char as our 'data'
        allocation.data = static_cast<char>(C3D::Random.Generate<u32>(65, 90));

        // Ensure that we have gotten a valid pointer
        ExpectNotEqual(nullptr, allocation.dataPtr);

        // Check if the allocated alignment matches our requested alignment
        u16 allocatedAlignment = 0;
        Memory.GetAlignment(allocation.dataPtr, &allocatedAlignment);
        ExpectEqual(alignment, allocatedAlignment);

        // Fill our entire array with our randomly generated char for the entire size of our dataPtr
        std::fill_n(allocation.dataPtr, allocSize, allocation.data);
    }
}

template <u64 Size>
void CleanupAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator)
{
    for (auto& allocation : data)
    {
        allocator.Free(allocation.dataPtr);
    }
}

TEST(DynamicAllocatorShouldDoRandomSmallAllocationsAndFrees)
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory        = MebiBytes(16);
    constexpr u64 neededMemory        = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectEqual(usableMemory, allocator.FreeSpace());

    void* allocations[amountOfAllocations]{};

    for (auto& allocation : allocations)
    {
        // Generate a random size for our allocation between 4 bytes and 4 KibiBytes
        const auto allocSize = C3D::Random.Generate<u64>(4, KibiBytes(4));
        // Provided an alignment of 1 since we will ignore alignment for this test
        allocation = allocator.AllocateBlock(C3D::MemoryType::Test, allocSize, 1);
        ExpectNotEqual(nullptr, allocation);
    }

    for (const auto allocation : allocations)
    {
        allocator.Free(allocation);
    }

    allocator.Destroy();
    Memory.Free(memoryBlock);
}

template <u64 Size>
void IsDataCorrect(const std::array<AllocStruct, Size>& data)
{
    // Verify that all our data is still correct
    for (const auto& d : data)
    {
        if (d.size == 0) continue;

        // Check if the allocated alignment matches our expected alignment
        u16 allocatedAlignment = 0;
        Memory.GetAlignment(d.dataPtr, &allocatedAlignment);
        ExpectEqual(d.alignment, allocatedAlignment);

        for (u64 i = 0; i < d.size; i++)
        {
            ExpectEqual(d.data, d.dataPtr[i]);
        }
    }
}

TEST(DynamicAllocatorShouldHaveNoDataCorruption)
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory        = MebiBytes(16);
    constexpr u64 neededMemory        = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectEqual(usableMemory, allocator.FreeSpace());

    std::array<AllocStruct, amountOfAllocations> allocations{};

    MakeAllocations(allocations, allocator);

    // Verify that all our data is still correct
    IsDataCorrect(allocations);

    // Cleanup our data
    CleanupAllocations(allocations, allocator);

    allocator.Destroy();
    Memory.Free(memoryBlock);
}

template <u64 Size>
void FreeRandomAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator, const int freeCount)
{
    // Randomly pick some indices into the allocation array to free them
    const auto freeIndices = C3D::Random.GenerateMultiple<u64>(freeCount, 0, data.size() - 1);

    for (const auto i : freeIndices)
    {
        auto& alloc = data[i];
        if (alloc.size != 0)
        {
            allocator.Free(alloc.dataPtr);
            alloc.size    = 0;
            alloc.dataPtr = nullptr;
            alloc.data    = 0;
        }
    }
}

TEST(DynamicAllocatorShouldHaveNoDataCorruptionWithFrees)
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory        = MebiBytes(16);
    constexpr u64 neededMemory        = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectEqual(usableMemory, allocator.FreeSpace());

    std::array<AllocStruct, amountOfAllocations> allocations;

    // Make some allocations
    MakeAllocations(allocations, allocator);

    // Verify our memory
    IsDataCorrect(allocations);

    // Free ~800 random allocations
    FreeRandomAllocations(allocations, allocator, 800);

    // Verify our memory again
    IsDataCorrect(allocations);

    // Make some allocations
    MakeAllocations(allocations, allocator);

    // Verify our memory again
    IsDataCorrect(allocations);

    // Cleanup our data
    CleanupAllocations(allocations, allocator);

    allocator.Destroy();
    Memory.Free(memoryBlock);
}

void DynamicAllocator::RegisterTests(TestManager& manager)
{
    manager.StartType("Dynamic Allocator");
    REGISTER_TEST(DynamicAllocatorShouldCreateAndDestroy, "Dynamic Allocator should create and destroy.");
    REGISTER_TEST(DynamicAllocatorShouldDoRandomSmallAllocationsAndFrees,
                  "Dynamic Allocator should always allocate and free for lot's of random allocations");
    REGISTER_TEST(DynamicAllocatorShouldHaveNoDataCorruption, "Dynamic Allocator should always allocate without data corruption");
    REGISTER_TEST(DynamicAllocatorShouldHaveNoDataCorruptionWithFrees,
                  "Dynamic Allocator should always allocate and free without data corruption");
}
