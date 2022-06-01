
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
		RenderSystem();

		bool Init(Application* application);
		void Shutdown() const;

		// HACK: we should not expose this outside of the engine
		C3D_API void SetView(mat4 view);

		void OnResize(u16 width, u16 height);

		bool DrawFrame(const RenderPacket* packet) const;

		void CreateTexture(const u8* pixels, struct Texture* texture) const;
		bool CreateMaterial(struct Material* material) const;

		bool CreateGeometry(struct Geometry* geometry, u32 vertexSize, u32 vertexCount, const void* vertices,
			u32 indexSize, u32 indexCount, const void* indices) const;

		void DestroyTexture(struct Texture* texture) const;
		void DestroyMaterial(struct Material* material) const;
		void DestroyGeometry(struct Geometry* geometry) const;

	private:
		bool CreateBackend(RendererBackendType type);
		void DestroyBackend() const;

		LoggerInstance m_logger;

		// World
		mat4 m_projection, m_view;
		// UI
		mat4 m_uiProjection, m_uiView;

		f32 m_nearClip, m_farClip;

		RendererBackend* m_backend{ nullptr };
	};
}
