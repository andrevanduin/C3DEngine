
#pragma once
#include "../core/defines.h"

#include "renderer_types.h"
#include "renderer_backend.h"

namespace C3D
{
	class Application;

	struct Texture;
	struct Geometry;
	struct Shader;
	struct ShaderUniform;

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

		void CreateTexture(const u8* pixels, Texture* texture) const;
		void DestroyTexture(Texture* texture) const;


		bool CreateGeometry(Geometry* geometry, u32 vertexSize, u32 vertexCount, const void* vertices, u32 indexSize, u32 indexCount, const void* indices) const;
		void DestroyGeometry(Geometry* geometry) const;

		bool GetRenderPassId(const char* name, u8* outRenderPassId) const;

		bool CreateShader(Shader* shader, u8 renderPassId, u8 stageCount, char** stageFileNames, ShaderStage* stages);
		void DestroyShader(Shader* shader);

		bool InitializeShader(Shader* shader);

		bool UseShader(Shader* shader);

		bool ShaderBindGlobals(Shader* shader);
		bool ShaderBindInstance(Shader* shader, u32 instanceId);

		bool ShaderApplyGlobals(Shader* shader);
		bool ShaderApplyInstance(Shader* shader);

		bool AcquireShaderInstanceResources(Shader* shader, u32* outInstanceId);
		bool ReleaseShaderInstanceResources(Shader* shader, u32 instanceId);

		bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value);

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
