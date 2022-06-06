
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
		MaxType
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
	};

	class C3D_API MemorySystem : public System<MemorySystemConfig>
	{
	public:
		MemorySystem();

		bool Init(const MemorySystemConfig& config) override;
		void Shutdown() override;

		void* Allocate(u64 size, MemoryType type);

		template<class T>
		T* Allocate(MemoryType type);

		template<class T>
		T* Allocate(u64 count, MemoryType type);

		template<class T, class... Args>
		T* New(MemoryType type, Args&&... args);

		void Free(void* block, u64 size, MemoryType type);

		static void* Zero(void* block, u64 size);

		static void* Copy(void* dest, const void* source, u64 size);

		static void* Set(void* dest, i32 value, u64 size);

		string GetMemoryUsageString();

		[[nodiscard]] u64 GetAllocCount() const;
		[[nodiscard]] u64 GetMemoryUsage(MemoryType type) const;
	private:
		void* m_memory;

		MemoryStats m_stats;
		DynamicAllocator m_allocator;
	};

	template <class T, class... Args>
	T* MemorySystem::New(const MemoryType type, Args&&... args)
	{ 
		return new(Allocate(sizeof T, type)) T(std::forward<Args>(args)...);
	}

	template <class T>
	T* MemorySystem::Allocate(const MemoryType type)
	{
		return static_cast<T*>(Allocate(sizeof(T), type));
	}

	template <class T>
	T* MemorySystem::Allocate(const u64 count, const MemoryType type)
	{
		return static_cast<T*>(Allocate(sizeof(T) * count, type));
	}
}

