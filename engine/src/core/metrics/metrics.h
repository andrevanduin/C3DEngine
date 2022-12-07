
#pragma once
#include "core/defines.h"
#include "containers/array.h"
#include "types.h"

namespace C3D
{
#define Metrics C3D::MetricSystem::GetInstance()

#if defined C3D_MEMORY_METRICS && C3D_MEMORY_METRICS_POINTERS
#define MetricsAllocate(id, type, requested, required, ptr)			\
	Metrics.Allocate(id, { type, requested, required, ptr })
#elif C3D_MEMORY_METRICS
#define MetricsAllocate(id, type, requested, required, ptr)			\
	Metrics.Allocate(id, { type, requested, required })
#endif

#if defined C3D_MEMORY_METRICS && C3D_MEMORY_METRICS_POINTERS
#define MetricsFree(id, type, requested, required, ptr)				\
	Metrics.Free(id, { type, ptr })
#elif C3D_MEMORY_METRICS
#define MetricsFree(id, requested, required, type, ptr)				\
	Metrics.Allocate(id, { type, requested, required })
#endif

	constexpr auto AVG_COUNT = 30;
	constexpr auto METRICS_COUNT = 16;
	constexpr auto ALLOCATOR_NAME_MAX_LENGTH = 128;

	constexpr u8 EXTERNAL_ALLOCATOR_ID = 1;
	constexpr u8 GPU_ALLOCATOR_ID = 2;
	constexpr u8 DYNAMIC_ALLOCATOR_ID = 3;

	class C3D_API MetricSystem
	{
		static MetricSystem* s_instance;
	public:
		MetricSystem();

		void Init();

		void Update(f64 elapsedTime);

		/*
		 * @brief Creates an internal metrics object used for tracking allocators
		 * Returns an u8 id that is associated with this specific allocator
		*/
		u8 CreateAllocator(const char* name, AllocatorType type, u64 availableSpace);
		/*
		 * @brief Destroys the internal metrics object used for tracking allocators
		 * that is associated with the provided allocatorId
		*/
		void DestroyAllocator(u8 allocatorId);

		void Allocate(u8 allocatorId, const Allocation& a);

		void AllocateExternal(u64 size);

		void Free(u8 allocatorId, const DeAllocation& a);

		void FreeExternal(u64 size);

		void FreeAll(u8 allocatorId);

		void SetAllocatorAvailableSpace(u8 allocatorId, u64 availableSpace);

		[[nodiscard]] u64 GetAllocCount(u8 allocatorId = 0) const;

		[[nodiscard]] u64 GetMemoryUsage(MemoryType memoryType, u8 allocatorId = DYNAMIC_ALLOCATOR_ID) const;

		[[nodiscard]] u64 GetRequestedMemoryUsage(MemoryType memoryType, u8 allocatorId = DYNAMIC_ALLOCATOR_ID) const;

		void PrintMemoryUsage(u8 allocatorId);
		void PrintMemoryUsage();

		static MetricSystem& GetInstance();

		[[nodiscard]] f64 GetFps()			const { return m_fps; }
		[[nodiscard]] f64 GetFrameTime()	const { return m_msAverage; }

	private:
		static const char* SizeToText(u64 size, f64* outAmount);
		static void SprintfAllocation(const MemoryAllocations& allocation, int index, char* buffer, int& bytesWritten, int offset);

		u8 m_frameAverageCounter;

		f64 m_msTimes[AVG_COUNT];
		f64 m_msAverage;

		i32 m_frames;

		f64 m_accumulatedFrameMs;
		f64 m_fps;

		// The memory stats for all our different allocators
		Array<MemoryStats, METRICS_COUNT> m_memoryStats;
		// Keep track of the external allocations that we have no control over
		ExternalAllocations m_externalAllocations;
	};
}
