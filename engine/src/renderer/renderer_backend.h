
#pragma once
#include "../core/defines.h"
#include "core/logger.h"

#include "renderer_types.h"

namespace C3D
{
	class Application;

	class RendererBackend
	{
	public:
		explicit RendererBackend(const std::string& loggerName) : type(), state(), m_logger(loggerName) {}

		virtual bool Init(Application* application) = 0;
		virtual void Shutdown() = 0;

		virtual void OnResize(u16 width, u16 height) = 0;

		virtual bool BeginFrame(f32 deltaTime) = 0;

		virtual void UpdateGlobalWorldState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) = 0;
		virtual void UpdateGlobalUiState(mat4 projection, mat4 view, i32 mode) = 0;

		virtual void DrawGeometry(GeometryRenderData data) = 0;

		virtual bool EndFrame(f32 deltaTime) = 0;

		virtual bool BeginRenderPass(u8 renderPassId) = 0;
		virtual bool EndRenderPass(u8 renderPassId) = 0;

		virtual void CreateTexture(const u8* pixels, struct Texture* texture) = 0;
		virtual bool CreateMaterial(struct Material* material) = 0;

		virtual bool CreateGeometry(Geometry* geometry, u32 vertexSize, u32 vertexCount, const void* vertices, 
			u32 indexSize, u32 indexCount, const void* indices) = 0;

		virtual void DestroyTexture(struct Texture* texture) = 0;
		virtual void DestroyMaterial(struct Material* material) = 0;
		virtual void DestroyGeometry(Geometry* geometry) = 0;

		RendererBackendType type;
		RendererBackendState state;

	protected:
		LoggerInstance m_logger;
	};
}

