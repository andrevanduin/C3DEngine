
#pragma once
#include "defines.h"
#include "containers/array.h"

namespace C3D
{
#define Metrics C3D::MetricSystem::GetInstance()

	constexpr auto AVG_COUNT = 30;
	constexpr auto METRICS_COUNT = 16;
	constexpr auto ALLOCATOR_NAME_MAX_LENGTH = 128;

	constexpr u8 GPU_ALLOCATOR_ID = 1;
	constexpr u8 DYNAMIC_ALLOCATOR_ID = 2;

	enum class AllocatorType : u8
	{
		None,
		Dynamic,
		System,
		Linear,
		Malloc,
		Stack,
		GpuLocal,
		MaxType,
	};

	enum class MemoryType : u8
	{
		Unknown,
		DynamicAllocator,
		LinearAllocator,
		FreeList,
		Array,
		DynamicArray,
		HashTable,
		HashMap,
		RingQueue,
		Bst,
		String,
		C3DString,
		Application,
		ResourceLoader,
		Job,
		Texture,
		MaterialInstance,
		Geometry,
		CoreSystem,
		RenderSystem,
		Game,
		Transform,
		Entity,
		EntityNode,
		Scene,
		Shader,
		Resource,
		Vulkan,
		VulkanExternal,
		Direct3D,
		OpenGL,
		BitmapFont,
		SystemFont,
		Test,
		MaxType,
	};

	struct MemoryAllocation
	{
		u64 requestedSize;
		u64 requiredSize;

		u32 count;
	};

	using TaggedAllocations = Array<MemoryAllocation, static_cast<u64>(MemoryType::MaxType)>;

	struct MemoryStats
	{
		// The type of this allocator
		AllocatorType type;
		// The name of this allocator
		char name[128];
		// The amount of total space available in this allocator
		u64 totalAvailableSpace;
		// The amount of total space current required for all the allocations associated with this allocator
		u64 totalRequired;
		// The amount of total space requested by the user for this allocator
		u64 totalRequested;
		// The amount of total allocations currently done by this allocator
		u64 allocCount;
		// An array of all the different types of allocations with stats about each
		TaggedAllocations taggedAllocations;
	};

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

		void Allocate(u8 allocatorId, MemoryType type, u64 requestedSize);
		void Allocate(u8 allocatorId, MemoryType type, u64 requestedSize, u64 requiredSize);

		void Free(u8 allocatorId, MemoryType type, u64 requestedSize);
		void Free(u8 allocatorId, MemoryType type, u64 requestedSize, u64 requiredSize);

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
		static void SprintfAllocation(const MemoryAllocation& allocation, int index, char* buffer, int& bytesWritten, int offset);

		u8 m_frameAverageCounter;

		f64 m_msTimes[AVG_COUNT];
		f64 m_msAverage;

		i32 m_frames;

		f64 m_accumulatedFrameMs;
		f64 m_fps;

		Array<MemoryStats, METRICS_COUNT> m_memoryStats;
	};
}
