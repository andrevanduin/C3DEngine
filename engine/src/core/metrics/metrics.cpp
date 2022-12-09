
#include "metrics.h"

#include "core/c3d_string.h"
#include "core/logger.h"
#include "math/c3d_math.h"
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
		"HashMap          ",
		"RingQueue        ",
		"Bst              ",
		"String           ",
		"C3DString        ",
		"Application      ",
		"ResourceLoader   ",
		"Job              ",
		"Texture          ",
		"MaterialInstance ",
		"Geometry         ",
		"CoreSystem       ",
		"RenderSystem     ",
		"RenderView       ",
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
		"BitmapFont       ",
		"SystemFont       ",
		"Test             ",
	};

	MetricSystem* MetricSystem::s_instance;

	MetricSystem::MetricSystem()
		: m_frameAverageCounter(0), m_msTimes{}, m_msAverage(0), m_frames(0), m_accumulatedFrameMs(0), m_fps(0), m_memoryStats()
	{}

	void MetricSystem::Init()
	{
		// Invalidate all stats
		for (auto& stats : m_memoryStats)
		{
			stats.type = AllocatorType::None;
		}

		m_memoryStats[GPU_ALLOCATOR_ID].type = AllocatorType::GpuLocal;
		StringNCopy(m_memoryStats[GPU_ALLOCATOR_ID].name, "GPU_ALLOCATOR", ALLOCATOR_NAME_MAX_LENGTH);

		m_memoryStats[DYNAMIC_ALLOCATOR_ID].type = AllocatorType::Dynamic;
	}

	void MetricSystem::Update(const f64 elapsedTime)
	{
		// Calculate ms per frame average
		const f64 frameMs = elapsedTime * 1000.0;
		m_msTimes[m_frameAverageCounter] = frameMs;

		if (m_frameAverageCounter == AVG_COUNT - 1)
		{
			for (const auto msTime : m_msTimes)
			{
				m_msAverage += msTime;
			}

			m_msAverage /= AVG_COUNT;
		}

		m_frameAverageCounter++;
		m_frameAverageCounter %= AVG_COUNT;

		// Calculate average frames per second
		m_accumulatedFrameMs += frameMs;
		if (m_accumulatedFrameMs > 1000)
		{
			// At least 1 second has passed
			m_fps = m_frames;
			m_accumulatedFrameMs -= 1000;
			m_frames = 0;
		}

		// Count all frames
		m_frames++;
	}

	u8 SetMemoryStats(const char* name, MemoryStats& stats, const AllocatorType type, const u64 availableSpace, const u8 i)
	{
		stats.type = type;
		stats.totalAvailableSpace = availableSpace;
		StringNCopy(stats.name, name, ALLOCATOR_NAME_MAX_LENGTH);
		// Return the index into our array as an id
		return i;
	}

	u8 MetricSystem::CreateAllocator(const char* name, const AllocatorType type, const u64 availableSpace)
	{
		if (StringLength(name) > ALLOCATOR_NAME_MAX_LENGTH)
		{
			Logger::Fatal("Allocator name: '{}' should <= {} characters", name, ALLOCATOR_NAME_MAX_LENGTH);
		}

		if (type == AllocatorType::GlobalDynamic)
		{
			return SetMemoryStats(name, m_memoryStats[DYNAMIC_ALLOCATOR_ID], type, availableSpace, DYNAMIC_ALLOCATOR_ID);
		}
		if (type == AllocatorType::GpuLocal)
		{
			return SetMemoryStats(name, m_memoryStats[GPU_ALLOCATOR_ID], type, availableSpace, GPU_ALLOCATOR_ID);
		}

		for (u8 i = 0; i < METRICS_COUNT; i++)
		{
			auto& stats = m_memoryStats[i];
			if (stats.type == AllocatorType::None)
			{
				// Return the index into our array as an id
				return SetMemoryStats(name, stats, type, availableSpace, i);
			}
		}

		// If we got to this point we have no more space for metrics so we return an error
		Logger::Fatal("[METRICS] - Create() - Not enough space for Allocator metrics");
		return INVALID_ID_U8;
	}

	void MetricSystem::DestroyAllocator(const u8 allocatorId)
	{
		// Print the memory usage for this allocator
		PrintMemoryUsage(allocatorId, true);
		// Clear out the metrics we have on this allocator
		m_memoryStats[allocatorId] = MemoryStats();
	}

	void MetricSystem::Allocate(const u8 allocatorId, const Allocation& a)
	{
		auto& stats = m_memoryStats[allocatorId];
		const auto type = ToUnderlying(a.type);

		stats.allocCount++;
		stats.totalRequested += a.requestedSize;
		stats.totalRequired += a.requiredSize;

#ifdef C3D_MEMORY_METRICS_POINTERS
		auto& tagged = stats.taggedAllocations[type];
		if (!a.ptr)
		{
			tagged.count++;
			tagged.requestedSize += a.requestedSize;
			tagged.requiredSize += a.requiredSize;
		}
		else
		{
			tagged.allocations.emplace_back(a.ptr, m_file, m_line, a.requestedSize, a.requiredSize);
		}
#else
		stats.taggedAllocations[type].requestedSize += a.requestedSize;
		stats.taggedAllocations[type].requiredSize += a.requiredSize;
		stats.taggedAllocations[type].count++;
#endif
	}

	void MetricSystem::AllocateExternal(const u64 size)
	{
		m_externalAllocations.count++;
		m_externalAllocations.size += size;
	}

	void MetricSystem::Free(const u8 allocatorId, const DeAllocation& a)
	{
		auto& stats = m_memoryStats[allocatorId];
		const auto type = ToUnderlying(a.type);

		stats.allocCount--;

		auto& tagged = stats.taggedAllocations[type];

#ifdef C3D_MEMORY_METRICS_POINTERS
		const auto allocIt = std::find_if(tagged.allocations.begin(), tagged.allocations.end(), [a](const TrackedAllocation& t) { return t.ptr == a.ptr; });
		if (allocIt != tagged.allocations.end())
		{
			const auto alloc = *allocIt;

			stats.totalRequested -= alloc.requestedSize;
			stats.totalRequired -= alloc.requiredSize;

			tagged.allocations.erase(allocIt);
		}
		else
		{
			throw std::bad_alloc();
		}
#else
		stats.totalRequested -= a.requestedSize;
		stats.totalRequired -= a.requiredSize;

		stats.taggedAllocations[type].requestedSize -= a.requestedSize;
		stats.taggedAllocations[type].requiredSize -= a.requiredSize;
		stats.taggedAllocations[type].count--;
#endif
	}

	void MetricSystem::FreeExternal(const u64 size)
	{
		m_externalAllocations.count--;
		m_externalAllocations.size -= size;
	}

	void MetricSystem::FreeAll(const u8 allocatorId)
	{
		auto& stats = m_memoryStats[allocatorId];
		stats.allocCount = 0;
		stats.totalRequested = 0;
		stats.totalRequired = 0;

		for (auto& taggedAllocation : stats.taggedAllocations)
		{
#ifdef C3D_MEMORY_METRICS_POINTERS
			taggedAllocation.allocations.clear();
#else
			taggedAllocation.requestedSize = 0;
			taggedAllocation.requiredSize = 0;
			taggedAllocation.count = 0;
#endif
		}
	}

	void MetricSystem::SetAllocatorAvailableSpace(const u8 allocatorId, const u64 availableSpace)
	{
		m_memoryStats[allocatorId].totalAvailableSpace = availableSpace;
	}

	u64 MetricSystem::GetAllocCount(const u8 allocatorId) const
	{
		return m_memoryStats[allocatorId].allocCount;
	}

	u64 MetricSystem::GetMemoryUsage(const MemoryType memoryType, const u8 allocatorId) const
	{
#ifdef C3D_MEMORY_METRICS_POINTERS
		u64 requiredSize = 0;
		for (const auto& alloc : m_memoryStats[allocatorId].taggedAllocations[ToUnderlying(memoryType)].allocations)
		{
			requiredSize += alloc.requiredSize;
		}
		return requiredSize;
#else
		return m_memoryStats[allocatorId].taggedAllocations[ToUnderlying(memoryType)].requiredSize;
#endif
	}

	u64 MetricSystem::GetRequestedMemoryUsage(const MemoryType memoryType, const u8 allocatorId) const
	{
#ifdef C3D_MEMORY_METRICS_POINTERS
		const auto tagged = m_memoryStats[allocatorId].taggedAllocations[ToUnderlying(memoryType)];

		u64 requestedSize = 0;
		for (const auto& alloc : tagged.allocations)
		{
			requestedSize += alloc.requestedSize;
		}
		// Return the requested size + the gpu allocator's requested size
		return requestedSize + tagged.requestedSize;
#else
		return m_memoryStats[allocatorId].taggedAllocations[ToUnderlying(memoryType)].requestedSize;
#endif
	}

	void MetricSystem::PrintMemoryUsage(const u8 allocatorId, const bool debugLines)
	{
		char buffer[4096];
		Platform::Zero(buffer, sizeof(char) * 4096);

		const auto& memStats = m_memoryStats[allocatorId];
		if (memStats.type != AllocatorType::None)
		{
			auto offset = 0;
			i32 bytesWritten = snprintf(buffer + offset, 8192, "%s with id: '%d' and type: '%d'\n", memStats.name, allocatorId, ToUnderlying(memStats.type));
			if (bytesWritten == -1)
			{
				Logger::Fatal("[METRICS] - Destroy() - Sprintf_s() failed with an error.");
			}

			offset += bytesWritten;

			for (u8 j = 0; j < ToUnderlying(MemoryType::MaxType); j++)
			{
				auto& allocation = memStats.taggedAllocations[j];
				SprintfAllocation(allocation, j, buffer, bytesWritten, offset, debugLines);
				offset += bytesWritten;
			}

			const auto required = memStats.totalRequired;
			const auto total = memStats.totalAvailableSpace;
			const auto percentage = static_cast<f64>(required) / static_cast<f64>(total) * 100.0;

			f64 requiredAmount, totalAmount;
			const char* requiredUnit = SizeToText(required, &requiredAmount);
			const char* totalUnit = SizeToText(total, &totalAmount);

			bytesWritten = snprintf(buffer + offset, 8192, "  %d total allocations using: %.2f %-3s of total: %.2f %-3s (%.2f%%)\n",
				static_cast<int>(memStats.allocCount), requiredAmount, requiredUnit, totalAmount, totalUnit, percentage);

			Logger::Info(buffer);
		}
	}

	void MetricSystem::PrintMemoryUsage(const bool debugLines)
	{
		Logger::Info("--------- MEMORY USAGE ---------");
		for (u8 i = 0; i < METRICS_COUNT; i++)
		{
			PrintMemoryUsage(i, debugLines);
		}
		Logger::Info("--------- MEMORY USAGE ---------");
	}

	MetricSystem& MetricSystem::GetInstance()
	{
		if (!s_instance)
		{
			s_instance = new MetricSystem();
			s_instance->Init();
		}
		return *s_instance;
	}

	const char* MetricSystem::SizeToText(const u64 size, f64* outAmount)
	{
		if (size >= GibiBytes(1))
		{
			*outAmount = static_cast<f64>(size) / GibiBytes(1);
			return "GiB";
		}

		if (size >= MebiBytes(1))
		{
			*outAmount = static_cast<f64>(size) / MebiBytes(1);
			return "MiB";
		}

		if (size >= KibiBytes(1))
		{
			*outAmount = static_cast<f64>(size) / KibiBytes(1);
			return "KiB";
		}

		*outAmount = static_cast<f64>(size);
		return "B";
	}

	void MetricSystem::SprintfAllocation(const MemoryAllocations& allocation, const int index, char* buffer, int& bytesWritten, const int offset, const bool debugLines)
	{
		f64 requestedAmount, requiredAmount;
		const char* requestedUnit = nullptr;
		const char* requiredUnit = nullptr;
		auto count = 0;

#ifdef C3D_MEMORY_METRICS_POINTERS
		u64 requestedSize = 0;
		u64 requiredSize = 0;

		for (const auto& alloc : allocation.allocations)
		{
			requestedSize += alloc.requestedSize;
			requiredSize += alloc.requiredSize;

			if (debugLines)
			{
				Logger::Debug("[File: {}, Line: {}]", alloc.file, alloc.line);
			}
		}
		
		requestedUnit = SizeToText(requestedSize, &requestedAmount);
		requiredUnit = SizeToText(requiredSize, &requiredAmount);
		count = static_cast<int>(allocation.allocations.size());
#else
		requestedUnit = SizeToText(allocation.requestedSize, &requestedAmount);
		requiredUnit = SizeToText(allocation.requiredSize, &requiredAmount);
		count = allocation.count;
#endif

		if (EpsilonEqual(requestedAmount, 0) && EpsilonEqual(requiredAmount, 0))
		{
			bytesWritten = 0;
			return;
		}

		if (!EpsilonEqual(requestedAmount, requiredAmount))
		{
			bytesWritten = snprintf(buffer + offset, 8192, "  %s: %4d using %6.2f %-3s | (%6.2f %-3s)\n", MEMORY_TYPE_STRINGS[index], count, requestedAmount, requestedUnit, requiredAmount, requiredUnit);
		}
		else
		{
			bytesWritten = snprintf(buffer + offset, 8192, "  %s: %4d using %6.2f %-3s\n", MEMORY_TYPE_STRINGS[index], count, requestedAmount, requestedUnit);
		}

		if (bytesWritten == -1)
		{
			Logger::Fatal("[METRICS] - Destroy() - Sprintf_s() failed with an error.");
		}
	}
}
