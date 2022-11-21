
#pragma once
#include "defines.h"
#include "containers/dynamic_array.h"

namespace C3D
{
	class Identifier
	{
	public:
		static void Init();
		static void Destroy();

		static u32 GetNewId(void* owner);

		static void ReleaseId(u32 id);

	private:
		static DynamicArray<void*> s_owners;
	};
}
