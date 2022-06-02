
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
		"Dictionary       ",
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
	};

	MemorySystem::MemorySystem()
		: Service("MEMORY"), m_memory(nullptr), m_config(), m_stats()
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

		m_stats.taggedAllocations[static_cast<u8>(MemoryType::DynamicAllocator)] = memoryRequirement;
		m_stats.taggedAllocations[static_cast<u8>(MemoryType::FreeList)] = FreeList::GetMemoryRequirements(config.totalAllocSize);

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
	}

	void* MemorySystem::Allocate(const u64 size, const MemoryType type)
	{
		if (type == MemoryType::Unknown)
		{
			m_logger.Warn("Allocate called using MemoryType::UNKNOWN");
		}

		m_stats.totalAllocated += size;
		m_stats.allocCount++;
		m_stats.taggedAllocations[static_cast<u8>(type)] += size;

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

		m_stats.totalAllocated -= size;
		m_stats.taggedAllocations[static_cast<u8>(type)] -= size;

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

		char buffer[8000] = "System's Dynamic Memory usage:\n";
		u64 offset = strlen(buffer);
		u8 i = 0;
		for (const u64 size : m_stats.taggedAllocations)
		{
			f64 amount = static_cast<f64>(size);
			char unit[4] = "XiB";
			if (size >= gib) 
			{
				unit[0] = 'G';
				amount /= static_cast<f64>(gib);
			}
			else if (size >= mib) 
			{
				unit[0] = 'M';
				amount /= static_cast<f64>(mib);
			}
			else if (size >= kib) 
			{
				unit[0] = 'K';
				amount /= static_cast<f64>(kib);
			}
			else
			{
				unit[0] = 'B';
				unit[1] = 0;
			}

			offset += snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", MEMORY_TYPE_STRINGS[i], amount, unit);
			i++;
		}
		return buffer;
	}

	u64 MemorySystem::GetAllocCount() const
	{
		return m_stats.allocCount;
	}
}
