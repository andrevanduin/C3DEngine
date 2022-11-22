
#include "memory.h"
#include "logger.h"
#include "platform/platform.h"

namespace C3D
{
	MemorySystem::MemorySystem()
		: System("MEMORY"), m_memory(nullptr), m_initialized(false), m_freeListMemorySize(0), m_stats()
	{}

	bool MemorySystem::Init(const MemorySystemConfig& config)
	{
		m_config = config;

		// Total size required for our dynamic allocator with the given totalSize
		const u64 memoryRequirement = DynamicAllocator::GetMemoryRequirements(config.totalAllocSize);
		m_memory = Platform::Allocate(memoryRequirement, false);
		if (!m_memory)
		{
			m_logger.Error("Allocating memory pool failed");
			return false;
		}

		// Keep track of the memory usage of our freeList so we can subtract it during shutdown for accurate stats
		m_freeListMemorySize = memoryRequirement - config.totalAllocSize;

		m_stats.allocCount = 0;
		m_stats.totalAllocated = 0;

		// Initialize tagged allocations with zero
		for (auto& alloc : m_stats.taggedAllocations)
		{
			alloc.count = 0;
			alloc.size = 0;
		}

		if (!config.excludeFromStats)
		{
			m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)] =	{ config.totalAllocSize, 1	};
			m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)] =			{ m_freeListMemorySize, 1	};
		}
		
		if (!m_allocator.Create(m_memory, memoryRequirement, config.totalAllocSize))
		{
			m_logger.Error("Failed to create dynamic allocator");
			return false;
		}

		m_initialized = true;
		return true;
	}

	void MemorySystem::Shutdown()
	{
		m_initialized = false;

		m_allocator.Destroy();
		Platform::Free(m_memory, false);

		// To get accurate stats at the end of the run we manually update the stats on our DynamicAllocator and FreeList
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)].count--;
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)].size -= m_config.totalAllocSize;

		m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)].count--;
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)].size -= m_freeListMemorySize;
	}

	void* MemorySystem::Allocate(const u64 size, const MemoryType type)
	{
		return AllocateAligned(size, 1, type);
	}

	void* MemorySystem::AllocateAligned(const u64 size, const u16 alignment, MemoryType type)
	{
		if (!m_initialized)
		{
			// If our memory system is not initialized we fallback to our platform's allocate
			return Platform::Allocate(size, true);
		}

		if (type == MemoryType::Unknown)
		{
			m_logger.Warn("AllocateAligned() - Called using MemoryType::UNKNOWN");
		}

		std::lock_guard allocLoc(m_mutex);

		m_stats.totalAllocated += size;
		m_stats.allocCount++;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size += size;
		m_stats.taggedAllocations[t].count++;

		// TODO: Memory alignment
		void* block = m_allocator.AllocateAligned(size, alignment);
		Platform::ZeroOutMemory(block, size);

		return block;
	}

	void MemorySystem::AllocateReport(const u64 size, MemoryType type)
	{
		std::lock_guard allocateLock(m_mutex);

		m_stats.totalAllocated += size;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size += size;
		m_stats.taggedAllocations[t].count++;
	}

	void MemorySystem::Free(void* block, const u64 size, const MemoryType type)
	{
		return FreeAligned(block, size, type);
	}

	void MemorySystem::FreeAligned(void* block, const u64 size, MemoryType type)
	{
		if (!m_initialized)
		{
			// If our memory system is not initialized we fallback to our platform's free
			Platform::Free(block, true);
			return;
		}

		if (type == MemoryType::Unknown)
		{
			m_logger.Warn("FreeAligned() - Called using MemoryType::UNKNOWN");
		}

		std::lock_guard freeLock(m_mutex);

		m_stats.totalAllocated -= size;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size -= size;
		m_stats.taggedAllocations[t].count--;

		if (!m_allocator.FreeAligned(block))
		{
			m_logger.Fatal("FreeAligned() - Failed to free memory with dynamic allocator.");
		}
	}

	void MemorySystem::FreeReport(const u64 size, MemoryType type)
	{
		std::lock_guard allocateLock(m_mutex);

		m_stats.totalAllocated -= size;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size -= size;
		m_stats.taggedAllocations[t].count--;
	}

	bool MemorySystem::GetSizeAlignment(void* block, u64* outSize, u16* outAlignment)
	{
		return DynamicAllocator::GetSizeAlignment(block, outSize, outAlignment);
	}

	bool MemorySystem::GetAlignment(void* block, u16* outAlignment)
	{
		return DynamicAllocator::GetAlignment(block, outAlignment);
	}

	bool MemorySystem::GetAlignment(const void* block, u16* outAlignment)
	{
		return DynamicAllocator::GetAlignment(block, outAlignment);
	}

	void* MemorySystem::Zero(void* block, const u64 size)
	{
		return Platform::ZeroOutMemory(block, size);
	}

	void* MemorySystem::Copy(void* dest, const void* source, const u64 size)
	{
		return Platform::CopyOverMemory(dest, source, size);
	}

	void* MemorySystem::Set(void* dest, const i32 value, const u64 size)
	{
		return Platform::SetMemory(dest, value, size);
	}

	Array<MemoryAllocation, static_cast<u64>(MemoryType::MaxType)> MemorySystem::GetTaggedAllocations() const
	{
		return m_stats.taggedAllocations;
	}

	u64 MemorySystem::GetAllocCount() const
	{
		return m_stats.allocCount;
	}

	u64 MemorySystem::GetMemoryUsage(const MemoryType type) const
	{
		return m_stats.taggedAllocations[ToUnderlying(type)].size;
	}

	u64 MemorySystem::GetFreeSpace() const
	{
		return m_allocator.FreeSpace();
	}

	u64 MemorySystem::GetTotalUsableSpace() const
	{
		return m_allocator.GetTotalUsableSize();
	}

	bool MemorySystem::IsInitialized() const
	{
		return m_initialized;
	}
}
