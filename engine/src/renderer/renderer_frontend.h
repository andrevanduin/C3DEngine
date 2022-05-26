
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"
#include "renderer_backend.h"

namespace C3D
{
	class Application;

	class Renderer
	{
	public:
		static bool Init(Application* application);
		static void Shutdown();

		static void OnResize(u16 width, u16 height);

		static bool DrawFrame(const RenderPacket* packet);
		static bool BeginFrame(f32 deltaTime);
		static bool EndFrame(f32 deltaTime);
		
	private:
		static bool CreateBackend(RendererBackendType type);
		static void DestroyBackend();

		static RendererBackend* m_backend;
	};
}
