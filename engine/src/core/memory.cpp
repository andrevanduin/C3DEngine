
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
	};

	MemorySystem::MemorySystem()
		: System("MEMORY"), m_memory(nullptr), m_stats()
	{}

	bool MemorySystem::Init(const MemorySystemConfig& config)
	{
		m_config = config;

		// Total size required for our dynamic allocator with the given totalSize
		const u64 memoryRequirement = DynamicAllocator::GetMemoryRequirements(config.totalAllocSize);
		// TODO: memory alignment
		m_memory = Platform::Allocate(memoryRequirement, false);
		if (!m_memory)
		{
			m_logger.Error("Allocating memory pool failed");
			return false;
		}

		m_stats.allocCount = 0;
		m_stats.totalAllocated = 0;
		Zero(m_stats.taggedAllocations, sizeof(MemoryAllocation) * ToUnderlying(MemoryType::MaxType));

		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)] = { memoryRequirement, 1 };

		if (!m_allocator.Create(config.totalAllocSize, m_memory))
		{
			m_logger.Error("Failed to create dynamic allocator");
			return false;
		}

		return true;
	}

	void MemorySystem::Shutdown()
	{
		m_allocator.Destroy();
		// TODO: alignment
		Platform::Free(m_memory, false);

		m_logger.Info("Memory Usage after free:\n {}", GetMemoryUsageString());
	}

	void* MemorySystem::Allocate(const u64 size, const MemoryType type)
	{
		if (type == MemoryType::Unknown)
		{
			m_logger.Warn("Allocate called using MemoryType::UNKNOWN");
		}

		std::lock_guard allocLoc(m_mutex);

		m_stats.totalAllocated += size;
		m_stats.allocCount++;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size += size;
		m_stats.taggedAllocations[t].count++;

		// TODO: Memory alignment
		void* block = m_allocator.Allocate(size);
		Platform::ZeroOutMemory(block, size);

		return block;
	}

	void MemorySystem::Free(void* block, const u64 size, const MemoryType type)
	{
		if (type == MemoryType::Unknown)
		{
			m_logger.Warn("Free called using MemoryType::UNKNOWN");
		}

		std::lock_guard freeLock(m_mutex);

		m_stats.totalAllocated -= size;

		const auto t = static_cast<u8>(type);
		m_stats.taggedAllocations[t].size -= size;
		m_stats.taggedAllocations[t].count--;

		if (!m_allocator.Free(block, size))
		{
			m_logger.Warn("Failed to free memory with dynamic allocator. Trying Platform::Free()");
			Platform::Free(block, false); // TODO: alignment
		}
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
}
