
#include "identifier.h"

namespace C3D
{
	DynamicArray<void*, MallocAllocator> Identifier::s_owners;

	void Identifier::Init()
	{
		s_owners = DynamicArray<void*, MallocAllocator>(100);
	}

	void Identifier::Destroy()
	{
		s_owners.Destroy();
	}

	u32 Identifier::GetNewId(void* owner)
	{
		for (u32 i = 1; i < s_owners.Size(); i++)
		{
			// If we find an existing free spot we take it.
			if (s_owners[i] == nullptr)
			{
				s_owners[i] = owner;
				return i;
			}
		}

		// If there are no existing free slots we push a new one
		s_owners.PushBack(owner);
		// Our id will be equal to the index of the newly added item (size - 1)
		return static_cast<u32>(s_owners.Size()) - 1;
	}

	void Identifier::ReleaseId(u32& id)
	{
		if (id > s_owners.Size())
		{
			Logger::Error("[IDENTIFIER] - ReleaseId() - Tried to release an id ({}) outside of range (1 - {}). Nothing was released.", id, s_owners.Size());
			return;
		}
		
		// Free this id so it's usable again
		s_owners[id] = nullptr;
		// Ensure the id is set to invalid
		id = INVALID_ID;
	}
}
