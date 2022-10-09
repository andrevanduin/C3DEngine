
#pragma once
#include "../core/defines.h"

#include "renderer_types.h"
#include "renderer_backend.h"
#include "render_buffer.h"
#include "resources/shader.h"

namespace C3D
{
	class Application;

	struct Texture;
	struct Geometry;
	struct Shader;
	struct ShaderUniform;

	class Camera;

	class RenderSystem
	{
	public:
		RenderSystem();

		bool Init(const Application* application);
		void Shutdown();

		void OnResize(u16 width, u16 height);

		bool DrawFrame(const RenderPacket* packet);

		void CreateTexture(const u8* pixels, Texture* texture) const;
		void CreateWritableTexture(Texture* texture) const;

		void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) const;
		void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) const;

		void DestroyTexture(Texture* texture) const;

		bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) const;
		void DestroyGeometry(Geometry* geometry) const;
		void DrawGeometry(const GeometryRenderData& data) const;

		RenderPass* GetRenderPass(const char* name) const;
		bool BeginRenderPass(RenderPass* pass, RenderTarget* target) const;
		bool EndRenderPass(RenderPass* pass) const;

		bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const;
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

		void CreateRenderTarget(u8 attachmentCount, Texture** attachments, RenderPass* pass, u32 width, u32 height, RenderTarget* outTarget) const;
		void DestroyRenderTarget(RenderTarget* target, bool freeInternalMemory) const;

		bool CreateRenderBuffer(RenderBufferType type, u64 totalSize, bool useFreelist, RenderBuffer* outBuffer) const;

		void RegenerateRenderTargets() const;

		[[nodiscard]] bool IsMultiThreaded() const;

	private:
		bool CreateBackend(RendererBackendType type);
		void DestroyBackend();

		LoggerInstance m_logger;

		u32 m_materialShaderId, m_uiShaderId, m_skyBoxShaderId;

		u8 m_windowRenderTargetCount;
		u32 m_frameBufferWidth, m_frameBufferHeight;

		RenderPass* m_worldRenderPass;
		RenderPass* m_uiRenderPass;
		RenderPass* m_skyBoxRenderPass;

		bool m_resizing;
		u8 m_framesSinceResize;

		RendererBackend* m_backend{ nullptr };
	};
}
