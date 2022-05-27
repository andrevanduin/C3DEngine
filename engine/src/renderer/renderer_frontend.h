
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"
#include "renderer_backend.h"

namespace C3D
{
	class Application;

	class RenderSystem
	{
	public:
		bool Init(Application* application);
		void Shutdown();

		void OnResize(u16 width, u16 height) const;

		bool DrawFrame(const RenderPacket* packet) const;

		[[nodiscard]] bool BeginFrame(f32 deltaTime) const;
		[[nodiscard]] bool EndFrame(f32 deltaTime) const;
		
	private:
		bool CreateBackend(RendererBackendType type);
		void DestroyBackend() const;

		RendererBackend* m_backend{ nullptr };
	};
}
