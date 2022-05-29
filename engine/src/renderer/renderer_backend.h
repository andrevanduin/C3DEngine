
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"

namespace C3D
{
	class Application;

	class RendererBackend
	{
	public:
		RendererBackend() : type(), state() {}

		virtual bool Init(Application* application, Texture* defaultDiffuse) = 0;
		virtual void Shutdown() = 0;

		virtual void OnResize(u16 width, u16 height) = 0;

		virtual bool BeginFrame(f32 deltaTime) = 0;

		virtual void UpdateGlobalState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) = 0;

		virtual void UpdateObject(GeometryRenderData data) = 0;

		virtual bool EndFrame(f32 deltaTime) = 0;

		virtual void CreateTexture(const string& name, bool autoRelease, i32 width, i32 height, i32 channelCount, const u8* pixels, bool hasTransparency, struct Texture* outTexture) = 0;

		virtual void DestroyTexture(struct Texture* texture) = 0;
		
		RendererBackendType type;
		RendererBackendState state;
	};
}

