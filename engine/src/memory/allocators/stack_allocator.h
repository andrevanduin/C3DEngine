
#pragma once
#include "core/defines.h"
#include "core/logger.h"
#include "base_allocator.h"
#include "platform/platform.h"

namespace C3D
{
	template <u64 Size>
	class __declspec(dllexport) StackAllocator final : public BaseAllocator
	{
	public:
		StackAllocator();

		void Create(const char* name);
		void Destroy();

		void* AllocateBlock(MemoryType type, u64 size, u16 alignment = 1) override;
		void Free(MemoryType type, void* block) override;

		void FreeAll();

		[[nodiscard]] static u64 GetTotalSize() { return Size; }
		[[nodiscard]] u64 GetAllocated() const { return m_allocated; }

	private:
		LoggerInstance m_logger;

		Array<char, Size> m_memory;
		u64 m_allocated;
	};

	template <u64 Size>
	StackAllocator<Size>::StackAllocator()
		: BaseAllocator(ToUnderlying(AllocatorType::Stack)), m_logger("STACK_ALLOCATOR"), m_allocated(0)
	{}

	template <u64 Size>
	void StackAllocator<Size>::Create(const char* name)
	{
		m_id = Metrics.CreateAllocator(name, AllocatorType::Stack, Size);
		Platform::Zero(m_memory.Data(), Size);
	}

	template <u64 SizeKb>
	void StackAllocator<SizeKb>::Destroy()
	{
		FreeAll();
	}

	template <u64 Size>
	void* StackAllocator<Size>::AllocateBlock(const MemoryType type, const u64 size, u16 alignment)
	{
		if (m_allocated + size > Size)
		{
			m_logger.Error("Out of memory");
			throw std::bad_alloc();
		}

		const auto dataPtr = &m_memory[m_allocated];
		m_allocated += size;

		Metrics.Allocate(m_id, type, size);

		return dataPtr;
	}

	template <u64 Size>
	void StackAllocator<Size>::Free(MemoryType type, void* block) {}

	template <u64 Size>
	void StackAllocator<Size>::FreeAll()
	{
		Platform::Zero(m_memory.Data(), Size);
		m_allocated = 0;

		Metrics.FreeAll(m_id);
	}
}
