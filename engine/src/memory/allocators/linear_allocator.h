
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "base_allocator.h"

namespace C3D
{
	class C3D_API LinearAllocator final : public BaseAllocator
	{
	public:
		LinearAllocator();

		void Create(const char* name, u64 totalSize, void* memory = nullptr);
		void Destroy();

		void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) override;
		void Free(MemoryType type, void* block) override;

		void FreeAll();

		[[nodiscard]] u64 GetTotalSize() const { return m_totalSize; }
		[[nodiscard]] u64 GetAllocated() const { return m_allocated; }

		[[nodiscard]] bool OwnsMemory() const { return m_ownsMemory; }

	private:
		LoggerInstance<32> m_logger;

		u64 m_totalSize;
		u64 m_allocated;

		bool m_ownsMemory;
	};
}
