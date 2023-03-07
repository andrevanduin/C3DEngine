
#include "dynamic_allocator_tests.h"

#include <algorithm>
#include <array>

#include "../expect.h"
#include "../util.h"

#include <memory/allocators/dynamic_allocator.h>

#include <core/defines.h>
#include <memory/global_memory_system.h>

#include "core/metrics/metrics.h"

struct AllocStruct
{
    AllocStruct() : dataPtr(nullptr), data(0), size(0), alignment(0) {}

    char* dataPtr;
    char data;

    u64 size;
    u16 alignment;
};

u8 DynamicAllocatorShouldCreateAndDestroy()
{
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

	ExpectShouldBe(neededMemory, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicAllocator));

    allocator.Destroy();

    Memory.Free(C3D::MemoryType::DynamicAllocator, memoryBlock);

    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicAllocator));

    return true;
}

constexpr u16 POSSIBLE_ALIGNMENTS[3] = { 1, 4, 8 };

template <u64 Size>
bool MakeAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator, Util& util)
{
    for (auto& allocation : data)
    {
        // Skip if allocation is already made for this index
        if (allocation.size != 0) continue;

        // Generate an index in our possible alignments array
        const auto alignmentIndex = util.GenerateRandom<u64>(0, 2);
        const auto alignment = POSSIBLE_ALIGNMENTS[alignmentIndex];

        // Generate a random size for our allocation between 4 bytes and 4 KibiBytes
        auto allocSize = util.GenerateRandom<u64>(4, KibiBytes(4));

        // Ensure our randomly generator allocSize is divisible by our alignment
        while (allocSize % alignment != 0)
        {
            allocSize = util.GenerateRandom<u64>(4, KibiBytes(4));
        }

        // Keep track of the pointer to our data so we can check it later
        allocation.dataPtr = static_cast<char*>(allocator.AllocateBlock(C3D::MemoryType::Test, allocSize, alignment));
        // Also keep track of our alignment so we can verify it later on
        allocation.alignment = alignment;

        // Keep track of the size so we know how large the allocation should be
        allocation.size = allocSize;

        // Generate a random char as our 'data'
        allocation.data = static_cast<char>(util.GenerateRandom<u32>(65, 90));

        // Ensure that we have gotten a valid pointer
        ExpectShouldNotBe(nullptr, allocation.dataPtr);

        // Check if the allocated alignment matches our requested alignment
        u16 allocatedAlignment = 0;
        C3D::DynamicAllocator::GetAlignment(allocation.dataPtr, &allocatedAlignment);
        ExpectShouldBe(alignment, allocatedAlignment);

        // Fill our entire array with our randomly generated char for the entire size of our dataPtr
        std::fill_n(allocation.dataPtr, allocSize, allocation.data);
    }

    return true;
}

template <u64 Size>
void CleanupAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator)
{
	for (auto& allocation : data)
	{
        allocator.Free(C3D::MemoryType::Test, allocation.dataPtr);
	}
}

u8 DynamicAllocatorShouldDoRandomSmallAllocationsAndFrees()
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    Util util;

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    void* allocations[amountOfAllocations]{};

    for (auto& allocation : allocations)
    {
        // Generate a random size for our allocation between 4 bytes and 4 KibiBytes
        const auto allocSize = util.GenerateRandom<u64>(4, KibiBytes(4));
        // Provided an alignment of 1 since we will ignore alignment for this test
        allocation = allocator.AllocateBlock(C3D::MemoryType::Test, allocSize, 1);
        ExpectShouldNotBe(nullptr, allocation);
    }

    for (const auto allocation : allocations)
    {
        allocator.Free(C3D::MemoryType::Test, allocation);
    }

    allocator.Destroy();
    Memory.Free(C3D::MemoryType::DynamicAllocator, memoryBlock);

	return true;
}

template <u64 Size>
bool IsDataCorrect(const std::array<AllocStruct, Size>& data)
{
    // Verify that all our data is still correct
    for (const auto& d : data)
    {
        if (d.size == 0) continue;

        // Check if the allocated alignment matches our expected alignment
        u16 allocatedAlignment = 0;
        C3D::DynamicAllocator::GetAlignment(d.dataPtr, &allocatedAlignment);
        ExpectShouldBe(d.alignment, allocatedAlignment);

        for (u64 i = 0; i < d.size; i++)
        {
            ExpectShouldBe(d.data, d.dataPtr[i]);
        }
    }
    return true;
}

u8 DynamicAllocatorShouldHaveNoDataCorruption()
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    Util util;

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    std::array<AllocStruct, amountOfAllocations> allocations{};

    if (!MakeAllocations(allocations, allocator, util)) return false;

    // Verify that all our data is still correct
    if (!IsDataCorrect(allocations)) return false;

    // Cleanup our data
    CleanupAllocations(allocations, allocator);

    allocator.Destroy();
    Memory.Free(C3D::MemoryType::DynamicAllocator, memoryBlock);

    return true;
}

template <u64 Size>
bool FreeRandomAllocations(std::array<AllocStruct, Size>& data, C3D::DynamicAllocator& allocator, Util& util, const int freeCount)
{
    // Randomly pick some indices into the allocation array to free them
    const auto freeIndices = util.GenerateRandom<u64>(freeCount, 0, data.size() - 1);

    for (const auto i : freeIndices)
    {
        auto& alloc = data[i];
        if (alloc.size != 0)
        {
            allocator.Free(C3D::MemoryType::Test, alloc.dataPtr);
            alloc.size = 0;
            alloc.dataPtr = nullptr;
            alloc.data = 0;
        }
    }

    return true;
}

u8 DynamicAllocatorShouldHaveNoDataCorruptionWithFrees()
{
    constexpr u64 amountOfAllocations = 4000;
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.AllocateBlock(C3D::MemoryType::DynamicAllocator, neededMemory);

    Util util;

    C3D::DynamicAllocator allocator;
    allocator.Create(memoryBlock, neededMemory, usableMemory);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    std::array<AllocStruct, amountOfAllocations> allocations;

    // Make some allocations
    if (!MakeAllocations(allocations, allocator, util)) return false;

    // Verify our memory
    if (!IsDataCorrect(allocations)) return false;

    // Free ~800 random allocations
    if (!FreeRandomAllocations(allocations, allocator, util, 800)) return false;

    // Verify our memory again
    if (!IsDataCorrect(allocations)) return false;

    // Make some allocations
    if (!MakeAllocations(allocations, allocator, util)) return false;

    // Verify our memory again
    if (!IsDataCorrect(allocations)) return false;

    // Cleanup our data
    CleanupAllocations(allocations, allocator);

    allocator.Destroy();
    Memory.Free(C3D::MemoryType::DynamicAllocator, memoryBlock);

    return true;
}

void DynamicAllocator::RegisterTests(TestManager* manager)
{
    manager->StartType("Dynamic Allocator");
    manager->Register(DynamicAllocatorShouldCreateAndDestroy, "Dynamic Allocator should create and destroy.");
    manager->Register(DynamicAllocatorShouldDoRandomSmallAllocationsAndFrees, "Dynamic Allocator should always allocate and free for lot's of random allocations");
    manager->Register(DynamicAllocatorShouldHaveNoDataCorruption, "Dynamic Allocator should always allocate without data corruption");
    manager->Register(DynamicAllocatorShouldHaveNoDataCorruptionWithFrees, "Dynamic Allocator should always allocate and free without data corruption");
}
