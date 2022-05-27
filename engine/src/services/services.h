
#pragma once
#include "core/defines.h"
#include "memory/linear_allocator.h"

// Systems
#include "core/memory.h"
#include "core/input.h"
#include "core/events/event.h"

namespace C3D
{
	class C3D_API Services
	{
	public:
		static bool Init();
		static void Shutdown();
		static InputSystem&  Input()	{ return *m_pInput;  }
		static EventSystem&  Event()	{ return *m_pEvents; }

	private:
		static EventSystem* m_pEvents;
		static InputSystem* m_pInput;

		static LinearAllocator m_allocator;
	};
}

