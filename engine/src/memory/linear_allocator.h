
#pragma once
#include "core/defines.h"

namespace C3D
{
	class C3D_API LinearAllocator
	{
	public:
		void Create(u64 totalSize, void* memory);
		void Destroy();

		void* Allocate(u64 size);
		void FreeAll();

		[[nodiscard]] u64 GetTotalSize() const { return m_totalSize; }
		[[nodiscard]] u64 GetAllocated() const { return m_allocated; }

		[[nodiscard]] void* GetMemory() const { return m_memory; }
		[[nodiscard]] bool OwnsMemory() const { return m_ownsMemory; }
	private:
		u64 m_totalSize = 0;
		u64 m_allocated = 0;

		void* m_memory = nullptr;
		bool m_ownsMemory = false;
	};
}