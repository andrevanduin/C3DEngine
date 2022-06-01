
#pragma once
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	class C3D_API LinearAllocator
	{
	public:
		LinearAllocator();

		void Create(u64 totalSize, void* memory = nullptr);
		
		void Destroy();

		void* Allocate(u64 size);

		template<class T>
		T* New()
		{
			return new(Allocate(sizeof T)) T();
		}

		void FreeAll();

		[[nodiscard]] u64 GetTotalSize() const { return m_totalSize; }
		[[nodiscard]] u64 GetAllocated() const { return m_allocated; }

		[[nodiscard]] void* GetMemory() const { return m_memory; }
		[[nodiscard]] bool OwnsMemory() const { return m_ownsMemory; }

	private:
		LoggerInstance m_logger;

		u64 m_totalSize;
		u64 m_allocated;

		void* m_memory;
		bool m_ownsMemory;
	};
}
