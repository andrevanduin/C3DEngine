
#pragma once
#include "defines.h"

namespace C3D
{
	enum class MemoryType : u8
	{
		Unknown,
		Array,
		DynamicArray,
		Dictionary,
		RingQueue,
		Bst,
		String,
		Application,
		Job,
		Texture,
		MaterialInstance,
		Renderer,
		Game,
		Transform,
		Entity,
		EntityNode,
		Scene,
		MaxType
	};

	struct MemoryStats
	{
		u64 totalAllocated;
		u64 taggedAllocations[static_cast<u8>(MemoryType::MaxType)];
	};

	class C3D_API Memory
	{
	public:
		static void Init();
		static void Shutdown();

		static void* Allocate(u64 size, MemoryType type);

		template<class T>
		static T* Allocate(u64 count, MemoryType type);

		template<class T, class... Args>
		static T* New(MemoryType type, Args&&... args);

		static void  Free(void* block, u64 size, MemoryType type);

		static void* Zero(void* block, u64 size);

		static void* Copy(void* dest, const void* source, u64 size);

		static void* Set(void* dest, i32 value, u64 size);

		static string GetMemoryUsageString();
	private:
		static MemoryStats m_stats;
	};

	template <class T, class... Args>
	T* Memory::New(const MemoryType type, Args&&... args)
	{ 
		return new(Allocate(sizeof T, type)) T(std::forward<Args>(args)...);
	}

	template <class T>
	T* Memory::Allocate(const u64 count, const MemoryType type)
	{
		return static_cast<T*>(Allocate(sizeof(T) * count, type));
	}
}

