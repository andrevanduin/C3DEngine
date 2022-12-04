
#pragma once
#include "core/defines.h"
#include "core/metrics.h"

namespace C3D
{
	class BaseAllocator
	{
	public:
		explicit BaseAllocator(const u8 type)
			: m_id(INVALID_ID_U8), m_type(type), m_memoryBlock(nullptr)
		{}

		BaseAllocator(const BaseAllocator&) = delete;
		BaseAllocator(BaseAllocator&&) = delete;

		BaseAllocator& operator=(const BaseAllocator&) = delete;
		BaseAllocator& operator=(BaseAllocator&&) = delete;

		virtual ~BaseAllocator() = default;

		virtual void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) = 0;
		virtual void Free(MemoryType type, void* block) = 0;

		template <typename T>
		T* Allocate(const MemoryType type, const u64 count = 1)
		{
			return static_cast<T*>(AllocateBlock(type, sizeof(T) * count, alignof(T)));
		}

		template <class T, class... Args>
		T* New(const MemoryType type, Args&&... args)
		{
			return new(AllocateBlock(type, sizeof T, alignof(T))) T(std::forward<Args>(args)...);
		}

		[[nodiscard]] void* GetMemory() const { return m_memoryBlock; }

		[[nodiscard]] u8 GetId() const { return m_id; }

	protected:
		// The id for this allocator
		u8 m_id;
		u8 m_type;
		
		char* m_memoryBlock;
	};
}
