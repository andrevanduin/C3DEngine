
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"

struct SDL_Window;

namespace C3D
{
	class Application;

	class RendererBackend
	{
	public:
		bool virtual Init(Application* application) { return true; }
		void virtual Shutdown() {}

		void virtual OnResize(u16 width, u16 height) {}

		bool virtual BeginFrame(f32 deltaTime)	{ return true; }
		bool virtual EndFrame(f32 deltaTime)	{ return true; }

		RendererBackendType type;
		RendererBackendState state;
	};
}

