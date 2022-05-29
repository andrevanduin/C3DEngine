
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"
#include "renderer_backend.h"
#include "core/memory.h"
#include "core/events/event_callback.h"

#include "core/events/event_context.h"

namespace C3D
{
	class Application;

	class RenderSystem
	{
	public:
		RenderSystem();

		bool Init(Application* application);
		void Shutdown();

		// HACK: we should not expose this outside of the engine
		C3D_API void SetView(mat4 view);

		void OnResize(u16 width, u16 height);

		bool DrawFrame(const RenderPacket* packet);

		[[nodiscard]] bool BeginFrame(f32 deltaTime) const;
		[[nodiscard]] bool EndFrame(f32 deltaTime) const;

		void CreateTexture(const string& name, bool autoRelease, i32 width, i32 height, i32 channelCount, const u8* pixels, bool hasTransparency, struct Texture* outTexture) const;
		void DestroyTexture(struct Texture* texture) const;
		
	private:
		bool CreateBackend(RendererBackendType type);
		void DestroyBackend() const;

		static void CreateDefaultTexture(Texture* texture);
		bool LoadTexture(const string& name, Texture* texture) const;

		// TEMP
		bool OnDebugEvent(u16 code, void* sender, EventContext data);

		IEventCallback* m_onDebugEventCallback;
		// TEMP END

		mat4 m_projection, m_view;
		f32 m_nearClip, m_farClip;

		Texture m_defaultTexture;
		// TEMP
		Texture m_testDiffuseTexture;
		// TEMP END

		RendererBackend* m_backend{ nullptr };
	};
}
