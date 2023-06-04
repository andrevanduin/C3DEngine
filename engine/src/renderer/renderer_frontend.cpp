
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/engine.h"

#include "renderer/vulkan/renderer_vulkan.h"
#include "resources/shader.h"

#include "systems/system_manager.h"

#include "platform/platform.h"

#include "systems/resources/resource_system.h"
#include "systems/render_views/render_view_system.h"

namespace C3D
{
	RenderSystem::RenderSystem(const Engine* engine)
		: BaseSystem(engine, "RENDERER"), m_windowRenderTargetCount(0), m_frameBufferWidth(1280), m_frameBufferHeight(720),
		  m_resizing(false), m_framesSinceResize(0), m_backend(nullptr)
	{}

	bool RenderSystem::Init()
	{
		// TODO: Make this configurable once we have multiple rendering backend options
		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			m_logger.Error("Init() - Failed to create Vulkan Renderer Backend");
			return false;
		}

		m_logger.Info("Created Vulkan Renderer Backend");

		// TODO: Expose this to the application to configure.
		RendererBackendConfig backendConfig{};
		backendConfig.applicationName = "TestEnv";
		backendConfig.engine = m_engine;
		backendConfig.flags = FlagVSyncEnabled | FlagPowerSavingEnabled;

		if (!m_backend->Init(backendConfig, &m_windowRenderTargetCount))
		{
			m_logger.Error("Init() - Failed to Initialize Renderer Backend.");
			return false;
		}

		m_logger.Info("Initialized Vulkan Renderer Backend");
		return true;
	}

	void RenderSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");

		m_backend->Shutdown();
		DestroyBackend();
	}

	void RenderSystem::OnResize(const u16 width, const u16 height)
	{
		m_resizing = true;
		m_frameBufferWidth = width;
		m_frameBufferHeight = height;
		m_framesSinceResize = 0;
	}

	bool RenderSystem::DrawFrame(RenderPacket* packet)
	{
		m_backend->state.frameNumber++;

		if (m_resizing)
		{
			m_framesSinceResize++;

			if (m_framesSinceResize >= 30)
			{
				// Notify our views of the resize
				Views.OnWindowResize(m_frameBufferWidth, m_frameBufferHeight);
				m_backend->OnResize(m_frameBufferWidth, m_frameBufferHeight);

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

		if (m_backend->BeginFrame(packet->deltaTime))
		{
			const u8 attachmentIndex = m_backend->GetWindowAttachmentIndex();

			// Render each view
			for (auto& viewPacket : packet->views)
			{
				if (!Views.OnRender(viewPacket.view, &viewPacket, m_backend->state.frameNumber, attachmentIndex))
				{
					m_logger.Error("DrawFrame() - Failed on calling OnRender() for view: '{}'.", viewPacket.view->name);
					return false;
				}
			}

			// End frame
			if (!m_backend->EndFrame(packet->deltaTime))
			{
				m_logger.Error("DrawFrame() - EndFrame() failed");
				return false;
			}
		}

		return true;
	}

	void RenderSystem::SetViewport(const vec4& rect) const
	{
		m_backend->SetViewport(rect);
	}

	void RenderSystem::ResetViewport() const
	{
		m_backend->ResetViewport();
	}

	void RenderSystem::SetScissor(const vec4& rect) const
	{
		m_backend->SetScissor(rect);
	}

	void RenderSystem::ResetScissor() const
	{
		m_backend->ResetScissor();
	}

	void RenderSystem::SetLineWidth(const float lineWidth) const
	{
		m_backend->SetLineWidth(lineWidth);
	}

	void RenderSystem::CreateTexture(const u8* pixels, Texture* texture) const
	{
		m_backend->CreateTexture(pixels, texture);
	}

	void RenderSystem::CreateWritableTexture(Texture* texture) const
	{
		m_backend->CreateWritableTexture(texture);
	}

	void RenderSystem::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight) const
	{
		m_backend->ResizeTexture(texture, newWidth, newHeight);
	}

	void RenderSystem::WriteDataToTexture(Texture* texture, const u32 offset, const u32 size, const u8* pixels) const
	{
		m_backend->WriteDataToTexture(texture, offset, size, pixels);
	}

	void RenderSystem::ReadDataFromTexture(Texture* texture, const u32 offset, const u32 size, void** outMemory) const
	{
		m_backend->ReadDataFromTexture(texture, offset, size, outMemory);
	}

	void RenderSystem::ReadPixelFromTexture(Texture* texture, const u32 x, const u32 y, u8** outRgba) const
	{
		m_backend->ReadPixelFromTexture(texture, x, y, outRgba);
	}

	bool RenderSystem::CreateGeometry(Geometry* geometry, const u32 vertexSize, const u64 vertexCount, const void* vertices,
	                                  const u32 indexSize, const u64 indexCount, const void* indices) const
	{
		return m_backend->CreateGeometry(geometry, vertexSize, vertexCount, vertices, indexSize, indexCount, indices);
	}

	void RenderSystem::DestroyTexture(Texture* texture) const
	{
		return m_backend->DestroyTexture(texture);
	}

	void RenderSystem::DestroyGeometry(Geometry* geometry) const
	{
		return m_backend->DestroyGeometry(geometry);
	}

	void RenderSystem::DrawGeometry(const GeometryRenderData& data) const
	{
		return m_backend->DrawGeometry(data);
	}

	bool RenderSystem::BeginRenderPass(RenderPass* pass, RenderTarget* target) const
	{
		return m_backend->BeginRenderPass(pass, target);
	}

	bool RenderSystem::EndRenderPass(RenderPass* pass) const
	{
		return m_backend->EndRenderPass(pass);
	}

	bool RenderSystem::CreateShader(Shader* shader, const ShaderConfig& config, RenderPass* pass) const
	{
		return m_backend->CreateShader(shader, config, pass);
	}

	void RenderSystem::DestroyShader(Shader& shader) const
	{
		return m_backend->DestroyShader(shader);
	}

	bool RenderSystem::InitializeShader(Shader* shader) const
	{
		return m_backend->InitializeShader(shader);
	}

	bool RenderSystem::UseShader(Shader* shader) const
	{
		return m_backend->UseShader(shader);
	}

	bool RenderSystem::ShaderBindGlobals(Shader* shader) const
	{
		return m_backend->ShaderBindGlobals(shader);
	}

	bool RenderSystem::ShaderBindInstance(Shader* shader, u32 instanceId) const
	{
		return m_backend->ShaderBindInstance(shader, instanceId);
	}

	bool RenderSystem::ShaderApplyGlobals(Shader* shader) const
	{
		return m_backend->ShaderApplyGlobals(shader);
	}

	bool RenderSystem::ShaderApplyInstance(Shader* shader, const bool needsUpdate) const
	{
		return m_backend->ShaderApplyInstance(shader, needsUpdate);
	}

	bool RenderSystem::AcquireShaderInstanceResources(Shader* shader, TextureMap** maps, u32* outInstanceId) const
	{
		return m_backend->AcquireShaderInstanceResources(shader, maps, outInstanceId);
	}

	bool RenderSystem::ReleaseShaderInstanceResources(Shader* shader, const u32 instanceId) const
	{
		return m_backend->ReleaseShaderInstanceResources(shader, instanceId);
	}

	bool RenderSystem::AcquireTextureMapResources(TextureMap* map) const
	{
		return m_backend->AcquireTextureMapResources(map);
	}

	void RenderSystem::ReleaseTextureMapResources(TextureMap* map) const
	{
		m_backend->ReleaseTextureMapResources(map);
	}

	bool RenderSystem::SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) const
	{
		return m_backend->SetUniform(shader, uniform, value);
	}

	void RenderSystem::CreateRenderTarget(const u8 attachmentCount, RenderTargetAttachment* attachments, RenderPass* pass, const u32 width, const u32 height, RenderTarget* outTarget) const
	{
		m_backend->CreateRenderTarget(attachmentCount, attachments, pass, width, height, outTarget);
	}

	void RenderSystem::DestroyRenderTarget(RenderTarget* target, const bool freeInternalMemory) const
	{
		m_backend->DestroyRenderTarget(target, freeInternalMemory);
		if (freeInternalMemory)
		{
			Platform::Zero(target, sizeof(RenderTarget));
		}
	}

	RenderPass* RenderSystem::CreateRenderPass(const RenderPassConfig& config) const
	{
		return m_backend->CreateRenderPass(config);
	}

	bool RenderSystem::DestroyRenderPass(RenderPass* pass) const
	{
		// Destroy this pass's render targets first
		for (u8 i = 0; i < pass->renderTargetCount; i++)
		{
			DestroyRenderTarget(&pass->targets[i], true);
		}
		return m_backend->DestroyRenderPass(pass);
	}

	Texture* RenderSystem::GetWindowAttachment(const u8 index) const
	{
		return m_backend->GetWindowAttachment(index);
	}

	Texture* RenderSystem::GetDepthAttachment(const u8 index) const
	{
		return m_backend->GetDepthAttachment(index);
	}

	u8 RenderSystem::GetWindowAttachmentIndex() const
	{
		return m_backend->GetWindowAttachmentIndex();
	}

	u8 RenderSystem::GetWindowAttachmentCount() const
	{
		return m_backend->GetWindowAttachmentCount();
	}

	RenderBuffer* RenderSystem::CreateRenderBuffer(const RenderBufferType type, const u64 totalSize, const bool useFreelist) const
	{
		return m_backend->CreateRenderBuffer(type, totalSize, useFreelist);
	}

	bool RenderSystem::DestroyRenderBuffer(RenderBuffer* buffer) const
	{
		return m_backend->DestroyRenderBuffer(buffer);
	}

	bool RenderSystem::IsMultiThreaded() const
	{
		return m_backend->IsMultiThreaded();
	}

	void RenderSystem::SetFlagEnabled(const RendererConfigFlagBits flag, const bool enabled) const
	{
		m_backend->SetFlagEnabled(flag, enabled);
	}

	bool RenderSystem::IsFlagEnabled(const RendererConfigFlagBits flag) const
	{
		return m_backend->IsFlagEnabled(flag);
	}

	bool RenderSystem::CreateBackend(const RendererBackendType type)
	{
		if (type == RendererBackendType::Vulkan)
		{
			m_backend = Memory.New<RendererVulkan>(MemoryType::RenderSystem);
			return true;
		}

		m_logger.Error("Tried Creating an Unsupported Renderer Backend: {}", ToUnderlying(type));
		return false;
	}

	void RenderSystem::DestroyBackend()
	{
		if (m_backend->type == RendererBackendType::Vulkan)
		{
			Memory.Delete(MemoryType::RenderSystem, m_backend);
			m_backend = nullptr;
		}
	}
}
