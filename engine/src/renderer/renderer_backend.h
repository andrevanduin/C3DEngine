
#pragma once
#include "../core/defines.h"
#include "core/logger.h"

#include "renderer_types.h"

namespace C3D
{
	class Application;
	struct Texture;
	struct Shader;
	struct ShaderUniform;

	class RendererBackend
	{
	public:
		explicit RendererBackend(const std::string& loggerName) : type(), state(), m_logger(loggerName) {}

		virtual bool Init(Application* application) = 0;
		virtual void Shutdown() = 0;

		virtual void OnResize(u16 width, u16 height) = 0;

		virtual bool BeginFrame(f32 deltaTime) = 0;

		virtual void DrawGeometry(GeometryRenderData data) = 0;

		virtual bool EndFrame(f32 deltaTime) = 0;

		virtual bool BeginRenderPass(u8 renderPassId) = 0;
		virtual bool EndRenderPass(u8 renderPassId) = 0;

		virtual void CreateTexture(const u8* pixels, Texture* texture) = 0;
		virtual bool CreateGeometry(Geometry* geometry, u32 vertexSize, u64 vertexCount, const void* vertices, u32 indexSize, u64 indexCount, const void* indices) = 0;

		virtual void DestroyTexture(Texture* texture) = 0;
		virtual void DestroyGeometry(Geometry* geometry) = 0;

		virtual bool CreateShader(Shader* shader, u8 renderPassId, const std::vector<char*>& stageFileNames, const std::vector<ShaderStage>& stages) = 0;
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

		RendererBackendType type;
		RendererBackendState state;

	protected:
		LoggerInstance m_logger;
	};
}

