
#pragma once
#include "core/defines.h"
#include "memory/linear_allocator.h"

// Systems
#include "core/input.h"
#include "core/events/event.h"
#include "renderer/renderer_frontend.h"

#define Input		C3D::Services::GetInput()
#define Event		C3D::Services::GetEvent()
#define Renderer	C3D::Services::GetRenderer()

namespace C3D
{
	class Application;

	class C3D_API Services
	{
	public:
		static bool Init(Application* application);
		static void Shutdown();

		static InputSystem&  GetInput()		{ return *m_pInput;		}
		static EventSystem&  GetEvent()		{ return *m_pEvents;	}
		static RenderSystem& GetRenderer()	{ return *m_pRenderer;	}

	private:
		static EventSystem*  m_pEvents;
		static InputSystem*  m_pInput;
		static RenderSystem* m_pRenderer;

		static LinearAllocator m_allocator;
	};
}

