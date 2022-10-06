
#include "memory.h"
#include "logger.h"
#include "platform/platform.h"

namespace C3D
{
	static const char* MEMORY_TYPE_STRINGS[static_cast<u8>(MemoryType::MaxType)] =
	{
		"Unknown          ",
		"Dynamic_Allocator",
		"Linear_Allocator ",
		"FreeList         ",
		"Array            ",
		"DynamicArray     ",
		"HashTable        ",
		"RingQueue        ",
		"Bst              ",
		"String           ",
		"Application      ",
		"ResourceLoader   ",
		"Job              ",
		"Texture          ",
		"MaterialInstance ",
		"Geometry         ",
		"RenderSystem     ",
		"Game             ",
		"Transform        ",
		"Entity           ",
		"EntityNode       ",
		"Scene            ",
		"Shader           ",
		"Resource         ",
		"Vulkan           ",
		"VulkanExternal   ",
		"Direct3D         ",
		"OpenGL           ",
		"GpuLocal         ",
	};

	MemorySystem::MemorySystem()
		: System("MEMORY"), m_memory(nullptr), m_stats()
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
		Zero(m_stats.taggedAllocations, sizeof(MemoryAllocation) * ToUnderlying(MemoryType::MaxType));

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

		return true;
	}

	void MemorySystem::Shutdown()
	{
		m_allocator.Destroy();
		Platform::Free(m_memory, false);

		// To get accurate stats at the end of the run we manually update the stats on our DynamicAllocator and FreeList
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)].count--;
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)].size -= m_config.totalAllocSize;

		m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)].count--;
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)].size -= m_freeListMemorySize;

		m_logger.Info("Memory Usage after free:\n {}", GetMemoryUsageString());
	}

	void* MemorySystem::Allocate(const u64 size, const MemoryType type)
	{
		return AllocateAligned(size, 1, type);
	}

	void* MemorySystem::AllocateAligned(const u64 size, const u16 alignment, MemoryType type)
	{
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
			m_logger.Fatal("FreeAligned() - Failed to free memory with dynamic allocator. Trying Platform::Free()");
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

	string MemorySystem::GetMemoryUsageString()
	{
		constexpr u64 kib = 1024;
		constexpr u64 mib = kib * 1024;
		constexpr u64 gib = mib * 1024;

		char buffer[2000] = "System's Dynamic Memory usage:\n";
		u64 offset = strlen(buffer);
		u8 i = 0;
		for (const auto& allocation : m_stats.taggedAllocations)
		{
			f64 amount = static_cast<f64>(allocation.size);
			char unit[4] = "XiB";
			if (allocation.size >= gib)
			{
				unit[0] = 'G';
				amount /= static_cast<f64>(gib);
			}
			else if (allocation.size >= mib)
			{
				unit[0] = 'M';
				amount /= static_cast<f64>(mib);
			}
			else if (allocation.size >= kib)
			{
				unit[0] = 'K';
				amount /= static_cast<f64>(kib);
			}
			else
			{
				unit[0] = 'B';
				unit[1] = 0;
			}

			offset += snprintf(buffer + offset, 2000, "  %s: %.2f%s (%d)\n", MEMORY_TYPE_STRINGS[i], amount, unit, allocation.count);
			i++;
		}
		return buffer;
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
}
