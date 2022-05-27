
#pragma once
#include "core/defines.h"
#include "memory/linear_allocator.h"

// Systems
#include "core/input.h"
#include "core/events/event.h"
#include "renderer/renderer_frontend.h"

#define INPUT C3D::Services::Input()
#define EVENT C3D::Services::Event()

namespace C3D
{
	class Application;

	class C3D_API Services
	{
	public:
		static bool Init(Application* application);
		static void Shutdown();

		static InputSystem&  Input()	{ return *m_pInput;		}
		static EventSystem&  Event()	{ return *m_pEvents;	}
		static RenderSystem& Renderer()	{ return *m_pRenderer;	}

	private:
		static EventSystem*  m_pEvents;
		static InputSystem*  m_pInput;
		static RenderSystem* m_pRenderer;

		static LinearAllocator m_allocator;
	};
}

