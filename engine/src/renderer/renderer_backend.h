
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"
#include "vertex.h"

namespace C3D
{
	class Application;

	class RendererBackend
	{
	public:
		RendererBackend() : type(), state() {}

		virtual bool Init(Application* application) = 0;
		virtual void Shutdown() = 0;

		virtual void OnResize(u16 width, u16 height) = 0;

		virtual bool BeginFrame(f32 deltaTime) = 0;

		virtual void UpdateGlobalState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) = 0;

		virtual void DrawGeometry(GeometryRenderData data) = 0;

		virtual bool EndFrame(f32 deltaTime) = 0;

		virtual void CreateTexture(const u8* pixels, struct Texture* texture) = 0;
		virtual bool CreateMaterial(struct Material* material) = 0;
		virtual bool CreateGeometry(Geometry* geometry, u32 vertexCount, const Vertex3D* vertices, u32 indexCount, const u32* indices) = 0;

		virtual void DestroyTexture(struct Texture* texture) = 0;
		virtual void DestroyMaterial(struct Material* material) = 0;
		virtual void DestroyGeometry(Geometry* geometry) = 0;

		RendererBackendType type;
		RendererBackendState state;
	};
}

