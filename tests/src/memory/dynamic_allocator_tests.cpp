
#include "dynamic_allocator_tests.h"

#include <algorithm>
#include <random>

#include "../expect.h"

#include <memory/dynamic_allocator.h>
#include <containers/freelist.h>

#include <core/defines.h>
#include <core/memory.h>

#include <services/services.h>

u8 DynamicAllocatorShouldCreateAndDestroy()
{
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.Allocate(neededMemory, C3D::MemoryType::DynamicAllocator);

    C3D::DynamicAllocator allocator;
    allocator.Create(usableMemory, memoryBlock);

    ExpectShouldBe(neededMemory, Memory.GetMemoryUsage(C3D::MemoryType::DynamicAllocator));

    allocator.Destroy();

    ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::DynamicAllocator));

    return true;
}

u8 DynamicAllocatorShouldReportCorrectFreeSpace()
{
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.Allocate(neededMemory, C3D::MemoryType::DynamicAllocator);

    C3D::DynamicAllocator allocator;
    allocator.Create(usableMemory, memoryBlock);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    for (auto i = 0; i < 64; i++)
    {
        constexpr u64 allocSize = 1024;
        allocator.Allocate(allocSize);

        ExpectShouldBe(usableMemory - (allocSize * (i + 1)), allocator.FreeSpace());
    }

    allocator.Destroy();
    return true;
}

u8 DynamicAllocatorShouldAllocAndFree()
{
    constexpr u64 amountOfAllocations = 10;
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.Allocate(neededMemory, C3D::MemoryType::DynamicAllocator);

    C3D::DynamicAllocator allocator;
    allocator.Create(usableMemory, memoryBlock);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    u8* allocations[amountOfAllocations]{};
    constexpr u64 allocSize = MebiBytes(1);

    auto freeSpace = usableMemory;
    for (auto& allocation : allocations)
    {
	    allocation = static_cast<u8*>(allocator.Allocate(allocSize));
        freeSpace -= allocSize;

        ExpectShouldBe(freeSpace, allocator.FreeSpace());
    }

    for (auto& allocation : allocations)
    {
        ExpectShouldBe(true, allocator.Free(allocation, allocSize));
        freeSpace += allocSize;

        ExpectShouldBe(freeSpace, allocator.FreeSpace());
    }

    allocator.Destroy();
    return true;
}

template <u64 Size>
struct Data
{
	explicit Data(const u64 size)
    {
        for (auto i = 0; i < size; i++)
        {
            data[i] = 'A';
        }
    }

    char data[Size];
};

u8 DynamicAllocatorShouldDoRandomAllocations()
{
    constexpr u64 usableMemory = MebiBytes(16);
    constexpr u64 neededMemory = C3D::DynamicAllocator::GetMemoryRequirements(usableMemory);

    const auto memoryBlock = Memory.Allocate(neededMemory, C3D::MemoryType::DynamicAllocator);

    C3D::DynamicAllocator allocator;
    allocator.Create(usableMemory, memoryBlock);

    ExpectShouldBe(usableMemory, allocator.FreeSpace());

    std::vector<std::pair<u8*, u64>> allocations;
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(5)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(15)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(35)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(2)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(3)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(17)));
    allocations.emplace_back(std::make_pair(nullptr, MebiBytes(2)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(24)));
    allocations.emplace_back(std::make_pair(nullptr, MebiBytes(5)));
    allocations.emplace_back(std::make_pair(nullptr, KibiBytes(105)));

    auto freeSpace = usableMemory;
    auto charI = 0;

    // Do 10 random allocations
    for (auto& allocation : allocations)
    {
        allocation.first = static_cast<u8*>(allocator.Allocate(allocation.second));
        for (u64 i = 0; i < allocation.second; i++)
        {
            allocation.first[i] = 'A' + charI;
        }

        charI++;
        freeSpace -= allocation.second;
    }

    //std::shuffle(allocations.begin(), allocations.end(), std::random_device());

    // Do 5 frees
    for (auto i = 3; i < 8; i++)
    {
        auto& allocation = allocations[i];

        ExpectShouldBe(true, allocator.Free(allocation.first, allocation.second));
        allocation.first = nullptr;

        freeSpace += allocation.second;

        ExpectShouldBe(freeSpace, allocator.FreeSpace());
    }

    // Do 3 more allocations
    for (auto i = 3; i < 6; i++)
    {
        auto& allocation = allocations[i];
        allocation.first = static_cast<u8*>(allocator.Allocate(allocation.second));
        for (u64 j = 0; j < allocation.second; j++)
        {
            allocation.first[j] = 'A' + i;
        }
    }

    allocator.Destroy();
    return true;
}

struct MemAllocTestObject
{
	
};

void DynamicAllocator::RegisterTests(TestManager* manager)
{
    manager->Register(DynamicAllocatorShouldCreateAndDestroy, "Dynamic Allocator should create and destroy.");
    manager->Register(DynamicAllocatorShouldReportCorrectFreeSpace, "Dynamic Allocator should always report the correct amount of free space.");
    manager->Register(DynamicAllocatorShouldAllocAndFree, "Dynamic Allocator should always allocate and free the correct amount of space.");
    manager->Register(DynamicAllocatorShouldDoRandomAllocations, "Dynamic Allocator should always allocate and free correctly with random allocations.");
}
