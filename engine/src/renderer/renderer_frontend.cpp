
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"
#include "resources/resource_types.h"
#include "resources/shader.h"

#include "services/services.h"

#include "core/events/event.h"
#include "platform/platform.h"
#include "resources/loaders/shader_loader.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/render_view_system.h"

namespace C3D
{
	// TODO: Obtain ambient color from scene instead of hardcoding it here
	RenderSystem::RenderSystem()
		: m_logger("RENDERER"), m_materialShaderId(0), m_uiShaderId(0), m_windowRenderTargetCount(0), m_frameBufferWidth(1280), m_frameBufferHeight(720),
		  m_worldRenderPass(nullptr), m_uiRenderPass(nullptr), m_resizing(false), m_framesSinceResize(0)
	{}

	bool RenderSystem::Init(const Application* application)
	{
		// TODO: Make this configurable once we have multiple rendering backend options
		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			m_logger.Error("Init() - Failed to create Vulkan Renderer Backend");
			return false;
		}

		m_logger.Info("Created Vulkan Renderer Backend");

		RendererBackendConfig backendConfig{};
		backendConfig.applicationName = "TestEnv";
		backendConfig.frontend = this;
		backendConfig.renderPassCount = 3;
		backendConfig.window = application->GetWindow();

		constexpr auto skyboxRenderPassName = "RenderPass.Builtin.Skybox";
		constexpr auto worldRenderPassName = "RenderPass.Builtin.World";
		constexpr auto uiRenderPassName = "RenderPass.Builtin.UI";
		
		RenderPassConfig passConfigs[3];
		passConfigs[0].name = skyboxRenderPassName;
		passConfigs[0].previousName = nullptr;
		passConfigs[0].nextName = worldRenderPassName;
		passConfigs[0].renderArea = { 0, 0, 1280, 720 };
		passConfigs[0].clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };
		passConfigs[0].clearFlags = ClearColor;

		passConfigs[1].name = worldRenderPassName;
		passConfigs[1].previousName = skyboxRenderPassName;
		passConfigs[1].nextName = uiRenderPassName;
		passConfigs[1].renderArea = { 0, 0, 1280, 720 };
		passConfigs[1].clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };
		passConfigs[1].clearFlags = ClearDepth | ClearStencil;

		passConfigs[2].name = uiRenderPassName;
		passConfigs[2].previousName = worldRenderPassName;
		passConfigs[2].nextName = nullptr;
		passConfigs[2].renderArea = { 0, 0, 1280, 720 };
		passConfigs[2].clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };
		passConfigs[2].clearFlags = ClearNone;

		backendConfig.passConfigs = passConfigs;

		if (!m_backend->Init(backendConfig, &m_windowRenderTargetCount))
		{
			m_logger.Error("Init() - Failed to Initialize Renderer Backend.");
			return false;
		}

		// TODO: We will know how to get these when we define views.
		m_skyBoxRenderPass = m_backend->GetRenderPass(skyboxRenderPassName);
		m_skyBoxRenderPass->renderTargetCount = m_windowRenderTargetCount;
		m_skyBoxRenderPass->targets = Memory.Allocate<RenderTarget>(m_windowRenderTargetCount, MemoryType::Array);

		m_worldRenderPass = m_backend->GetRenderPass(worldRenderPassName);
		m_worldRenderPass->renderTargetCount = m_windowRenderTargetCount;
		m_worldRenderPass->targets = Memory.Allocate<RenderTarget>(m_windowRenderTargetCount, MemoryType::Array);

		m_uiRenderPass = m_backend->GetRenderPass(uiRenderPassName);
		m_uiRenderPass->renderTargetCount = m_windowRenderTargetCount;
		m_uiRenderPass->targets = Memory.Allocate<RenderTarget>(m_windowRenderTargetCount, MemoryType::Array);

		RegenerateRenderTargets();

		// Update the dimensions for our RenderPasses
		m_skyBoxRenderPass->renderArea = { 0, 0, m_frameBufferWidth, m_frameBufferHeight };
		m_worldRenderPass->renderArea = { 0, 0, m_frameBufferWidth, m_frameBufferHeight };
		m_uiRenderPass->renderArea = { 0, 0, m_frameBufferWidth, m_frameBufferHeight };

		// Shaders
		ShaderResource configResource{};
		// Get shader resources for skybox shader
		if (!Resources.Load(BUILTIN_SHADER_NAME_SKY_BOX, &configResource))
		{
			m_logger.Error("Init() - Failed to load builtin skybox shader config resource.");
			return false;
		}

		// Create our skybox shader
		if (!Shaders.Create(configResource.config))
		{
			m_logger.Error("Init() - Failed to create builtin skybox shader.");
			return false;
		}

		// Unload resources for skybox shader config
		Resources.Unload(&configResource);
		// Obtain the shader id belonging to our skybox shader
		m_skyBoxShaderId = Shaders.GetId(BUILTIN_SHADER_NAME_SKY_BOX);

		// Get shader resources for material shader
		if (!Resources.Load(BUILTIN_SHADER_NAME_MATERIAL, &configResource))
		{
			m_logger.Error("Init() - Failed to load builtin material shader config resource.");
			return false;
		}

		// Create our material shader
		if (!Shaders.Create(configResource.config))
		{
			m_logger.Error("Init() - Failed to create builtin material shader.");
			return false;
		}

		// Unload resources for material shader config
		Resources.Unload(&configResource);
		// Obtain the shader id belonging to our material shader
		m_materialShaderId = Shaders.GetId(BUILTIN_SHADER_NAME_MATERIAL);

		// Get shader resources for ui shader
		if (!Resources.Load(BUILTIN_SHADER_NAME_UI, &configResource))
		{
			m_logger.Error("Init() - Failed to load builtin ui shader config resource.");
			return false;
		}

		// Create our ui shader
		if (!Shaders.Create(configResource.config))
		{
			m_logger.Error("Init() - Failed to create builtin material shader.");
			return false;
		}

		// Unload resources for ui shader config
		Resources.Unload(&configResource);
		// Obtain the shader if belonging to our ui shader
		m_uiShaderId = Shaders.GetId(BUILTIN_SHADER_NAME_UI);

		m_logger.Info("Initialized Vulkan Renderer Backend");
		return true;
	}

	void RenderSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");

		for (u8 i = 0; i < m_windowRenderTargetCount; i++)
		{
			m_backend->DestroyRenderTarget(&m_skyBoxRenderPass->targets[i], true);
			m_backend->DestroyRenderTarget(&m_worldRenderPass->targets[i], true);
			m_backend->DestroyRenderTarget(&m_uiRenderPass->targets[i], true);
		}

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

	bool RenderSystem::DrawFrame(const RenderPacket* packet)
	{
		m_backend->state.frameNumber++;

		if (m_resizing)
		{
			m_framesSinceResize++;

			if (m_framesSinceResize >= 30)
			{
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

	void RenderSystem::CreateTexture(const u8* pixels, Texture* texture) const
	{
		return m_backend->CreateTexture(pixels, texture);
	}

	void RenderSystem::CreateWritableTexture(Texture* texture) const
	{
		return m_backend->CreateWritableTexture(texture);
	}

	void RenderSystem::ResizeTexture(Texture* texture, const u32 newWidth, const u32 newHeight) const
	{
		return m_backend->ResizeTexture(texture, newWidth, newHeight);
	}

	void RenderSystem::WriteDataToTexture(Texture* texture, const u32 offset, const u32 size, const u8* pixels) const
	{
		return m_backend->WriteDataToTexture(texture, offset, size, pixels);
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

	RenderPass* RenderSystem::GetRenderPass(const char* name) const
	{
		return m_backend->GetRenderPass(name);
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

	void RenderSystem::DestroyShader(Shader* shader) const
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
		return m_backend->ReleaseTextureMapResources(map);
	}

	bool RenderSystem::SetUniform(Shader* shader, const ShaderUniform* uniform, const void* value) const
	{
		return m_backend->SetUniform(shader, uniform, value);
	}

	void RenderSystem::CreateRenderTarget(const u8 attachmentCount, Texture** attachments, RenderPass* pass, const u32 width, const u32 height, RenderTarget* outTarget) const
	{
		return m_backend->CreateRenderTarget(attachmentCount, attachments, pass, width, height, outTarget);
	}

	void RenderSystem::DestroyRenderTarget(RenderTarget* target, const bool freeInternalMemory) const
	{
		return m_backend->DestroyRenderTarget(target, freeInternalMemory);
	}

	bool RenderSystem::CreateRenderBuffer(RenderBufferType type, u64 totalSize, bool useFreelist, RenderBuffer* outBuffer) const
	{
		return m_backend->CreateRenderBuffer(type, totalSize, useFreelist, outBuffer);
	}

	void RenderSystem::RegenerateRenderTargets() const
	{
		// Create render targets of each. TODO: Should be configurable
		for (u8 i = 0; i < m_windowRenderTargetCount; i++)
		{
			m_backend->DestroyRenderTarget(&m_skyBoxRenderPass->targets[i], false);
			m_backend->DestroyRenderTarget(&m_worldRenderPass->targets[i], false);
			m_backend->DestroyRenderTarget(&m_uiRenderPass->targets[i], false);

			Texture* windowTargetTexture = m_backend->GetWindowAttachment(i);
			Texture* depthTargetTexture = m_backend->GetDepthAttachment();

			// Skybox render targets
			Texture* skyboxAttachments[1] = { windowTargetTexture };
			m_backend->CreateRenderTarget(1, skyboxAttachments, m_skyBoxRenderPass, m_frameBufferWidth, m_frameBufferHeight, &m_skyBoxRenderPass->targets[i]);

			// World render targets
			Texture* attachments[2] = { windowTargetTexture, depthTargetTexture };
			m_backend->CreateRenderTarget(2, attachments, m_worldRenderPass, m_frameBufferWidth, m_frameBufferHeight, &m_worldRenderPass->targets[i]);

			// UI render targets
			Texture* uiAttachments[1] = { windowTargetTexture };
			m_backend->CreateRenderTarget(1, uiAttachments, m_uiRenderPass, m_frameBufferWidth, m_frameBufferHeight, &m_uiRenderPass->targets[i]);
		}
	}

	bool RenderSystem::IsMultiThreaded() const
	{
		return m_backend->IsMultiThreaded();
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
			Memory.Free(m_backend, sizeof(RendererVulkan), MemoryType::RenderSystem);
			m_backend = nullptr;
		}
	}
}
