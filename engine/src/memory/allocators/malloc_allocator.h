
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "base_allocator.h"

namespace C3D
{
	struct MallocAllocation
	{
		void* block;
		u64 size;
	};

	class C3D_API MallocAllocator final : public BaseAllocator
	{
	public:
		MallocAllocator();

		MallocAllocator(const MallocAllocator&) = delete;
		MallocAllocator(MallocAllocator&&) = delete;

		MallocAllocator& operator=(const MallocAllocator&) = delete;
		MallocAllocator& operator=(MallocAllocator&&) = delete;

		~MallocAllocator() override;

		void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) override;
		void Free(MemoryType type, void* block) override;

		[[nodiscard]] u64 GetTotalAllocated() const;

	private:
		LoggerInstance m_logger;
		Array<MallocAllocation, 32> m_allocations;
	};
}