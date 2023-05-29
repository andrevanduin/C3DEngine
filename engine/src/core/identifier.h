
#pragma once
#include "defines.h"
#include "containers/dynamic_array.h"

#include "memory/allocators/malloc_allocator.h"

namespace C3D
{
	class C3D_API Identifier
	{
	public:
		static void Init();

		static void Destroy();

		static u32 GetNewId(void* owner);

		static void ReleaseId(u32& id);

	private:
		static DynamicArray<void*, MallocAllocator> s_owners;
	};
}
