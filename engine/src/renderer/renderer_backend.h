
#pragma once
#include "../core/defines.h"
#include "core/logger.h"

#include "renderer_types.h"
#include "render_buffer.h"

namespace C3D
{
	class Application;
	struct Texture;
	struct Shader;
	struct ShaderUniform;
	struct ShaderConfig;

	class RendererBackend
	{
	public:
		explicit RendererBackend(const std::string& loggerName) : type(), state(), m_logger(loggerName) {}
		RendererBackend(const RendererBackend&) = delete;
		RendererBackend(RendererBackend&&) = delete;

		RendererBackend& operator=(const RendererBackend&) = delete;
		RendererBackend& operator=(RendererBackend&&) = delete;

		virtual ~RendererBackend() = default;

		virtual bool Init(const RendererBackendConfig& config, u8* outWindowRenderTargetCount) = 0;
		virtual void Shutdown() = 0;

		virtual void OnResize(u16 width, u16 height) = 0;

		virtual bool BeginFrame(f32 deltaTime) = 0;

		virtual void DrawGeometry(const GeometryRenderData& data) = 0;

		virtual bool EndFrame(f32 deltaTime) = 0;

		virtual bool BeginRenderPass(RenderPass* pass, RenderTarget* target) = 0;
		virtual bool EndRenderPass(RenderPass* pass) = 0;
		virtual RenderPass* GetRenderPass(const char* name) = 0;

		virtual void CreateTexture(const u8* pixels, Texture* texture) = 0;
		virtual void CreateWritableTexture(Texture* texture) = 0;

		virtual void WriteDataToTexture(Texture* texture, u32 offset, u32 size, const u8* pixels) = 0;
		virtual void ResizeTexture(Texture* texture, u32 newWidth, u32 newHeight) = 0;

		virtual void DestroyTexture(Texture* texture) = 0;

		virtual bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) = 0;
		virtual void DestroyGeometry(Geometry* geometry) = 0;

		virtual bool CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const = 0;
		virtual void DestroyShader(Shader* shader) = 0;

		virtual bool InitializeShader(Shader* shader) = 0;

		virtual bool UseShader(Shader* shader) = 0;

		virtual bool ShaderBindGlobals(Shader* shader) = 0;
		virtual bool ShaderBindInstance(Shader* shader, u32 instanceId) = 0;

		virtual bool ShaderApplyGlobals(Shader* shader) = 0;
		virtual bool ShaderApplyInstance(Shader* shader, bool needsUpdate) = 0;

		virtual bool AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId) = 0;
		virtual bool ReleaseShaderInstanceResources(Shader* shader, u32 instanceId) = 0;

		virtual bool AcquireTextureMapResources(TextureMap* map) = 0;
		virtual void ReleaseTextureMapResources(TextureMap* map) = 0;

		virtual bool SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) = 0;

		virtual void CreateRenderTarget(u8 attachmentCount, Texture** attachments, RenderPass* pass, u32 width, u32 height, RenderTarget* outTarget) = 0;
		virtual void DestroyRenderTarget(RenderTarget* target, bool freeInternalMemory) = 0;

		virtual bool CreateRenderBuffer(RenderBufferType type, u64 totalSize, bool useFreelist, RenderBuffer* outBuffer) = 0;

		virtual Texture* GetWindowAttachment(u8 index) = 0;
		virtual Texture* GetDepthAttachment() = 0;

		virtual u8 GetWindowAttachmentIndex() = 0;

		[[nodiscard]] virtual bool IsMultiThreaded() const = 0;

		RendererBackendType type;
		RendererBackendState state;

	protected:
		LoggerInstance m_logger;
	};
}

