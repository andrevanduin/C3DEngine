
#pragma once
#include "../core/defines.h"

#include "renderer_types.h"
#include "renderer_backend.h"
#include "core/events/event_context.h"

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
		void Shutdown();

		// HACK: we should not expose this outside of the engine
		C3D_API void SetView(mat4 view, vec3 viewPosition);

		void OnResize(u16 width, u16 height);

		bool DrawFrame(const RenderPacket* packet) const;

		void CreateTexture(const u8* pixels, Texture* texture) const;
		void DestroyTexture(Texture* texture) const;


		bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) const;
		void DestroyGeometry(Geometry* geometry) const;

		bool GetRenderPassId(const char* name, u8* outRenderPassId) const;

		bool CreateShader(Shader* shader, u8 renderPassId, const std::vector<char*>& stageFileNames, const std::vector<ShaderStage>& stages) const;
		void DestroyShader(Shader* shader) const;

		bool InitializeShader(Shader* shader) const;

		bool UseShader(Shader* shader) const;

		bool ShaderBindGlobals(Shader* shader) const;
		bool ShaderBindInstance(Shader* shader, u32 instanceId) const;

		bool ShaderApplyGlobals(Shader* shader) const;
		bool ShaderApplyInstance(Shader* shader, bool needsUpdate) const;

		bool AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId) const;
		bool ReleaseShaderInstanceResources(Shader* shader, u32 instanceId) const;

		bool AcquireTextureMapResources(TextureMap* map) const;
		void ReleaseTextureMapResources(TextureMap* map) const;

		bool SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) const;

	private:
		bool CreateBackend(RendererBackendType type);
		void DestroyBackend() const;

		bool OnEvent(u16 code, void* sender, EventContext context);

		LoggerInstance m_logger;

		// World
		mat4 m_projection, m_view;
		vec4 m_ambientColor;
		vec3 m_viewPosition;

		// UI
		mat4 m_uiProjection, m_uiView;

		f32 m_nearClip, m_farClip;

		u32 m_materialShaderId, m_uiShaderId;

		u32 m_renderMode;

		RendererBackend* m_backend{ nullptr };
	};
}
