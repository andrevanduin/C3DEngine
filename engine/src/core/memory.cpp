
#include "memory.h"
#include "logger.h"
#include "platform/platform.h"

namespace C3D
{
	MemoryStats Memory::m_stats;

	static const char* MEMORY_TYPE_STRINGS[static_cast<u8>(MemoryType::MaxType)] =
	{
		"Unknown         ",
		"Array           ",
		"Linear_Allocator",
		"DynamicArray    ",
		"Dictionary      ",
		"RingQueue       ",
		"Bst             ",
		"String          ",
		"Application     ",
		"Job             ",
		"Texture         ",
		"MaterialInstance",
		"Renderer        ",
		"Game            ",
		"Transform       ",
		"Entity          ",
		"EntityNode      ",
		"Scene           ",
	};

	void Memory::Init()
	{
		Platform::ZeroOutMemory(&m_stats, sizeof m_stats);
	}

	void Memory::Shutdown()
	{

	}

	void* Memory::Allocate(const u64 size, const MemoryType type)
	{
		if (type == MemoryType::Unknown)
		{
			Logger::Warn("Allocate called using MemoryType::UNKNOWN");
		}

		m_stats.totalAllocated += size;
		m_stats.allocCount++;
		m_stats.taggedAllocations[static_cast<u8>(type)] += size;

		// TODO: Memory alignment
		void* block = Platform::Allocate(size, false);
		Platform::ZeroOutMemory(block, size);
		return block;
	}

	void Memory::Free(void* block, const u64 size, const MemoryType type)
	{
		if (type == MemoryType::Unknown)
		{
			Logger::Warn("Free called using MemoryType::UNKNOWN");
		}

		m_stats.totalAllocated -= size;
		m_stats.taggedAllocations[static_cast<u8>(type)] -= size;

		//TODO: Memory alignment
		Platform::Free(block, false);
	}

	void* Memory::Zero(void* block, const u64 size)
	{
		return Platform::ZeroOutMemory(block, size);
	}

	void* Memory::Copy(void* dest, const void* source, const u64 size)
	{
		return Platform::CopyOverMemory(dest, source, size);
	}

	void* Memory::Set(void* dest, const i32 value, const u64 size)
	{
		return Platform::SetMemory(dest, value, size);
	}

	string Memory::GetMemoryUsageString()
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

	u64 Memory::GetAllocCount()
	{
		return m_stats.allocCount;
	}

}
