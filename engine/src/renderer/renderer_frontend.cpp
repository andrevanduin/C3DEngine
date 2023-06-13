
#include "renderer_frontend.h"

#include "platform/platform.h"

#include "core/logger.h"
#include "core/engine.h"

#include "resources/shader.h"

#include "systems/system_manager.h"
#include "systems/render_views/render_view_system.h"
#include "systems/cvars/cvar_system.h"
#include <core/function/function.h>

namespace C3D
{
	RenderSystem::RenderSystem(const Engine* engine)
		: SystemWithConfig(engine, "RENDERER"), m_windowRenderTargetCount(0), m_frameBufferWidth(1280), m_frameBufferHeight(720),
		  m_resizing(false), m_framesSinceResize(0), m_backendPlugin(nullptr)
	{}

	bool RenderSystem::Init(const RenderSystemConfig& config)
	{
		m_config = config;
		m_backendPlugin = config.rendererPlugin;
		if (!m_backendPlugin)
		{
			m_logger.Fatal("Init() - No valid renderer plugin provided");
			return false;
		}

		RendererPluginConfig rendererPluginConfig{};
		rendererPluginConfig.applicationName = m_config.applicationName;
		rendererPluginConfig.engine = m_engine;
		rendererPluginConfig.flags = m_config.flags;

		if (!m_backendPlugin->Init(rendererPluginConfig, &m_windowRenderTargetCount))
		{
			m_logger.Fatal("Init() - Failed to Initialize Renderer Backend.");
			return false;
		}

		auto& vSync = CVars.Get<bool>("vsync");
		vSync.AddOnChangedCallback([this](const bool& b) { SetFlagEnabled(FlagVSyncEnabled, b); });

		m_logger.Info("Init() - Successfully initialized Rendering System");
		return true;
	}

	void RenderSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");

		m_backendPlugin->Shutdown();
		Memory.Delete(MemoryType::RenderSystem, m_backendPlugin);
	}

	void RenderSystem::OnResize(const u32 width, const u32 height)
	{
		m_resizing = true;
		m_frameBufferWidth = width;
		m_frameBufferHeight = height;
		m_framesSinceResize = 0;
	}

	bool RenderSystem::DrawFrame(RenderPacket* packet)
	{
		m_backendPlugin->frameNumber++;

		if (m_resizing)
		{
			m_framesSinceResize++;

			if (m_framesSinceResize >= 5)
			{
				// Notify our views of the resize
				Views.OnWindowResize(m_frameBufferWidth, m_frameBufferHeight);
				m_backendPlugin->OnResize(m_frameBufferWidth, m_frameBufferHeight);

				m_framesSinceResize = 0;
				m_resizing = false;
			}
			else
			{
				// Simulate a 60FPS frame
				Platform::SleepMs(16);
				// Skip rendering this frame
				return true;
			}
		}

		if (m_backendPlugin->BeginFrame(packet->deltaTime))
		{
			const u8 attachmentIndex = m_backendPlugin->GetWindowAttachmentIndex();

			// Render each view
			for (auto& viewPacket : packet->views)
			{
				if (!Views.OnRender(viewPacket.view, &viewPacket, m_backendPlugin->frameNumber, attachmentIndex))
				{
					m_logger.Error("DrawFrame() - Failed on calling OnRender() for view: '{}'.", viewPacket.view->name);
					return false;
				}
			}

			// End frame
			if (!m_backendPlugin->EndFrame(packet->deltaTime))
			{
				m_logger.Error("DrawFrame() - EndFrame() failed");
				return false;
			}
		}

		return true;
	}

	void RenderSystem::SetViewport(const vec4& rect) const
	{
		m_backendPlugin->SetViewport(rect);
	}

	void RenderSystem::ResetViewport() const
	{
		m_backendPlugin->ResetViewport();
	}

	void RenderSystem::SetScissor(const vec4& rect) const
	{
		m_backendPlugin->SetScissor(rect);
	}

	void RenderSystem::ResetScissor() const
	{
		m_backendPlugin->ResetScissor();
	}

	void RenderSystem::SetLineWidth(const float lineWidth) const
	{
		m_backendPlugin->SetLineWidth(lineWidth);
	}

	void RenderSystem::CreateTexture(const u8* pixels, Texture* texture) const
	{
		m_backendPlugin->CreateTexture(pixels, texture);
	}

	void RenderSystem::CreateWritableTexture(Texture* texture) const
	{
		m_backendPlugin->CreateWritableTexture(texture);
	}

	void RenderSystem::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight) const
	{
		m_backendPlugin->ResizeTexture(texture, newWidth, newHeight);
	}

	void RenderSystem::WriteDataToTexture(Texture* texture, const u32 offset, const u32 size, const u8* pixels) const
	{
		m_backendPlugin->WriteDataToTexture(texture, offset, size, pixels);
	}

	void RenderSystem::ReadDataFromTexture(Texture* texture, const u32 offset, const u32 size, void** outMemory) const
	{
		m_backendPlugin->ReadDataFromTexture(texture, offset, size, outMemory);
	}

	void RenderSystem::ReadPixelFromTexture(Texture* texture, const u32 x, const u32 y, u8** outRgba) const
	{
		m_backendPlugin->ReadPixelFromTexture(texture, x, y, outRgba);
	}

	bool RenderSystem::CreateGeometry(Geometry* geometry, const u32 vertexSize, const u64 vertexCount, const void* vertices,
	                                  const u32 indexSize, const u64 indexCount, const void* indices) const
	{
		return m_backendPlugin->CreateGeometry(geometry, vertexSize, vertexCount, vertices, indexSize, indexCount, indices);
	}

	void RenderSystem::DestroyTexture(Texture* texture) const
	{
		return m_backendPlugin->DestroyTexture(texture);
	}

	void RenderSystem::DestroyGeometry(Geometry* geometry) const
	{
		return m_backendPlugin->DestroyGeometry(geometry);
	}

	void RenderSystem::DrawGeometry(const GeometryRenderData& data) const
	{
		return m_backendPlugin->DrawGeometry(data);
	}

	bool RenderSystem::BeginRenderPass(RenderPass* pass, RenderTarget* target) const
	{
		return m_backendPlugin->BeginRenderPass(pass, target);
	}

	bool RenderSystem::EndRenderPass(RenderPass* pass) const
	{
		return m_backendPlugin->EndRenderPass(pass);
	}

	bool RenderSystem::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
	{
		return m_backendPlugin->CreateShader(shader, config, pass);
	}

	void RenderSystem::DestroyShader(Shader& shader) const
	{
		return m_backendPlugin->DestroyShader(shader);
	}

	bool RenderSystem::InitializeShader(Shader* shader) const
	{
		return m_backendPlugin->InitializeShader(shader);
	}

	bool RenderSystem::UseShader(Shader* shader) const
	{
		return m_backendPlugin->UseShader(shader);
	}

	bool RenderSystem::ShaderBindGlobals(Shader* shader) const
	{
		return m_backendPlugin->ShaderBindGlobals(shader);
	}

	bool RenderSystem::ShaderBindInstance(Shader* shader, u32 instanceId) const
	{
		return m_backendPlugin->ShaderBindInstance(shader, instanceId);
	}

	bool RenderSystem::ShaderApplyGlobals(Shader* shader) const
	{
		return m_backendPlugin->ShaderApplyGlobals(shader);
	}

	bool RenderSystem::ShaderApplyInstance(Shader* shader, const bool needsUpdate) const
	{
		return m_backendPlugin->ShaderApplyInstance(shader, needsUpdate);
	}

	bool RenderSystem::AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId) const
	{
		return m_backendPlugin->AcquireShaderInstanceResources(shader, maps, outInstanceId);
	}

	bool RenderSystem::ReleaseShaderInstanceResources(Shader* shader, const u32 instanceId) const
	{
		return m_backendPlugin->ReleaseShaderInstanceResources(shader, instanceId);
	}

	bool RenderSystem::AcquireTextureMapResources(TextureMap* map) const
	{
		return m_backendPlugin->AcquireTextureMapResources(map);
	}

	void RenderSystem::ReleaseTextureMapResources(TextureMap* map) const
	{
		m_backendPlugin->ReleaseTextureMapResources(map);
	}

	bool RenderSystem::SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) const
	{
		return m_backendPlugin->SetUniform(shader, uniform, value);
	}

	void RenderSystem::CreateRenderTarget(const u8 attachmentCount, RenderTargetAttachment* attachments, RenderPass* pass, const u32 width, const u32 height, RenderTarget* outTarget) const
	{
		m_backendPlugin->CreateRenderTarget(attachmentCount, attachments, pass, width, height, outTarget);
	}

	void RenderSystem::DestroyRenderTarget(RenderTarget* target, const bool freeInternalMemory) const
	{
		m_backendPlugin->DestroyRenderTarget(target, freeInternalMemory);
		if (freeInternalMemory)
		{
			std::memset(target, 0, sizeof(RenderTarget));
		}
	}

	RenderPass* RenderSystem::CreateRenderPass(const RenderPassConfig& config) const
	{
		return m_backendPlugin->CreateRenderPass(config);
	}

	bool RenderSystem::DestroyRenderPass(RenderPass* pass) const
	{
		// Destroy this pass's render targets first
		for (u8 i = 0; i < pass->renderTargetCount; i++)
		{
			DestroyRenderTarget(&pass->targets[i], true);
		}
		return m_backendPlugin->DestroyRenderPass(pass);
	}

	Texture* RenderSystem::GetWindowAttachment(const u8 index) const
	{
		return m_backendPlugin->GetWindowAttachment(index);
	}

	Texture* RenderSystem::GetDepthAttachment(const u8 index) const
	{
		return m_backendPlugin->GetDepthAttachment(index);
	}

	u8 RenderSystem::GetWindowAttachmentIndex() const
	{
		return m_backendPlugin->GetWindowAttachmentIndex();
	}

	u8 RenderSystem::GetWindowAttachmentCount() const
	{
		return m_backendPlugin->GetWindowAttachmentCount();
	}

	RenderBuffer* RenderSystem::CreateRenderBuffer(const RenderBufferType type, const u64 totalSize, const bool useFreelist) const
	{
		return m_backendPlugin->CreateRenderBuffer(type, totalSize, useFreelist);
	}

	bool RenderSystem::DestroyRenderBuffer(RenderBuffer* buffer) const
	{
		return m_backendPlugin->DestroyRenderBuffer(buffer);
	}

	bool RenderSystem::IsMultiThreaded() const
	{
		return m_backendPlugin->IsMultiThreaded();
	}

	void RenderSystem::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled) const
	{
		m_backendPlugin->SetFlagEnabled(flag, enabled);
	}

	bool RenderSystem::IsFlagEnabled(const RendererConfigFlagBits flag) const
	{
		return m_backendPlugin->IsFlagEnabled(flag);
	}
}
