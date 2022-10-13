
#pragma once
#include "defines.h"

#include "systems/system.h"
#include "memory/dynamic_allocator.h"

namespace C3D
{
	enum class MemoryType : u8
	{
		Unknown,
		DynamicAllocator,
		LinearAllocator,
		FreeList,
		Array,
		DynamicArray,
		HashTable,
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
		GpuLocal,
		BitmapFont,
		SystemFont,
		MaxType,
	};

	struct MemoryAllocation
	{
		u64 size;
		u32 count;
	};

	struct MemoryStats
	{
		u64 totalAllocated;
		u64 allocCount;
		MemoryAllocation taggedAllocations[static_cast<u8>(MemoryType::MaxType)];
	};

	struct MemorySystemConfig
	{
		u64 totalAllocSize;
		bool excludeFromStats = false;
	};

	class C3D_API MemorySystem final : public System<MemorySystemConfig>
	{
	public:
		MemorySystem();

		bool Init(const MemorySystemConfig& config) override;
		void Shutdown() override;

		/* @brief Allocate a block of memory with the provided size. (Without alignment). */
		void* Allocate(u64 size, MemoryType type);
		/* @brief Allocate a block of memory with the provided size with alignment taken into account. */
		void* AllocateAligned(u64 size, u16 alignment, MemoryType type);

		/* @brief Reports an allocation associated with the application, but made externally.
		 * This can be done to track items that are allocated by 3rd party libraries but not actually allocate them ourselves.
		 */
		void AllocateReport(u64 size, MemoryType type);

		template<class T>
		T* Allocate(MemoryType type);

		template<class T>
		T* Allocate(u64 count, MemoryType type);

		template<class T, class... Args>
		T* New(MemoryType type, Args&&... args);

		/* @brief Frees a block of memory of the provided size without taking alignment into account. */
		void Free(void* block, u64 size, MemoryType type);

		/* @brief Frees a block of memory of the provided size aligned with the provided alignment. */
		void FreeAligned(void* block, u64 size, MemoryType type);

		/* @brief Reports a free associated with the application, but made externally.
		 * This can be done to track items that are freed by 3rd party libraries but not actually free them ourselves.
		 */
		void FreeReport(u64 size, MemoryType type);

		/* @brief Returns the size and alignment of the given block of memory. */
		static bool GetSizeAlignment(void* block, u64* outSize, u16* outAlignment);

		/* @brief Returns the alignment of the given block of memory. */
		static bool GetAlignment(void* block, u16* outAlignment);

		static bool GetAlignment(const void* block, u16* outAlignment);

		static void* Zero(void* block, u64 size);

		static void* Copy(void* dest, const void* source, u64 size);

		static void* Set(void* dest, i32 value, u64 size);

		[[nodiscard]] const MemoryAllocation* GetTaggedAllocations() const;

		[[nodiscard]] u64 GetAllocCount() const;
		[[nodiscard]] u64 GetMemoryUsage(MemoryType type) const;

		[[nodiscard]] u64 GetFreeSpace() const;
		[[nodiscard]] u64 GetTotalUsableSpace() const;

		[[nodiscard]] bool IsInitialized() const;

	private:
		void* m_memory;

		bool m_initialized;

		u64 m_freeListMemorySize;

		MemoryStats m_stats;
		DynamicAllocator m_allocator;

		// Mutex to enable multiThreaded use
		std::mutex m_mutex;
	};

	template <class T, class... Args>
	T* MemorySystem::New(const MemoryType type, Args&&... args)
	{ 
		return new(AllocateAligned(sizeof T, alignof(T), type)) T(std::forward<Args>(args)...);
	}

	template <class T>
	T* MemorySystem::Allocate(const MemoryType type)
	{
		return static_cast<T*>(AllocateAligned(sizeof(T), alignof(T), type));
	}

	template <class T>
	T* MemorySystem::Allocate(const u64 count, const MemoryType type)
	{
		return static_cast<T*>(AllocateAligned(sizeof(T) * count, alignof(T), type));
	}
}

