
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"
#include "core/c3d_string.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"
#include "resources/resource_types.h"
#include "resources/shader.h"

#include "services/services.h"

#include "core/events/event.h"
#include "resources/loaders/shader_loader.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"

namespace C3D
{
	// TODO: Obtain ambient color from scene instead of hardcoding it here
	RenderSystem::RenderSystem()
		: m_logger("RENDERER"), m_activeWorldCamera(nullptr), m_projection(), m_ambientColor(0.25, 0.25, 0.25, 1.0f),
		  m_uiProjection(), m_uiView(inverse(mat4(1))), m_nearClip(0.1f), m_farClip(1000.0f), m_materialShaderId(0), m_uiShaderId(0),
		  m_renderMode(0), m_windowRenderTargetCount(0), m_frameBufferWidth(1280), m_frameBufferHeight(720),
		  m_worldRenderPass(nullptr), m_uiRenderPass(nullptr), m_resizing(false), m_framesSinceResize(0)
	{
	}

	bool RenderSystem::Init(const Application* application)
	{
		// TODO: Make this configurable once we have multiple rendering backend options
		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			m_logger.Error("Init() - Failed to create Vulkan Renderer Backend");
			return false;
		}

		m_logger.Info("Created Vulkan Renderer Backend");

		Event.Register(SystemEventCode::SetRenderMode, new EventCallback(this, &RenderSystem::OnEvent));

		RendererBackendConfig backendConfig{};
		backendConfig.applicationName = "TestEnv";
		backendConfig.frontend = this;
		backendConfig.renderPassCount = 2;
		backendConfig.window = application->GetWindow();

		constexpr auto worldRenderPassName = "RenderPass.Builtin.World";
		constexpr auto uiRenderPassName = "RenderPass.Builtin.UI";

		RenderPassConfig passConfigs[2];
		passConfigs[0].name = worldRenderPassName;
		passConfigs[0].previousName = nullptr;
		passConfigs[0].nextName = uiRenderPassName;
		passConfigs[0].renderArea = { 0, 0, 1280, 720 };
		passConfigs[0].clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };
		passConfigs[0].clearFlags = ClearColor | ClearDepth | ClearStencil;

		passConfigs[1].name = uiRenderPassName;
		passConfigs[1].previousName = worldRenderPassName;
		passConfigs[1].nextName = nullptr;
		passConfigs[1].renderArea = { 0, 0, 1280, 720 };
		passConfigs[1].clearColor = { 0.0f, 0.0f, 0.2f, 1.0f };
		passConfigs[1].clearFlags = ClearNone;

		backendConfig.passConfigs = passConfigs;

		if (!m_backend->Init(backendConfig, &m_windowRenderTargetCount))
		{
			m_logger.Error("Init() - Failed to Initialize Renderer Backend.");
			return false;
		}

		// TODO: We will know how to get these when we define views.
		m_worldRenderPass = m_backend->GetRenderPass(worldRenderPassName);
		m_worldRenderPass->renderTargetCount = m_windowRenderTargetCount;
		m_worldRenderPass->targets = Memory.Allocate<RenderTarget>(m_windowRenderTargetCount, MemoryType::Array);

		m_uiRenderPass = m_backend->GetRenderPass(uiRenderPassName);
		m_uiRenderPass->renderTargetCount = m_windowRenderTargetCount;
		m_uiRenderPass->targets = Memory.Allocate<RenderTarget>(m_windowRenderTargetCount, MemoryType::Array);

		RegenerateRenderTargets();

		// Update the dimensions for our RenderPasses
		m_worldRenderPass->renderArea = { 0, 0, m_frameBufferWidth, m_frameBufferHeight };
		m_uiRenderPass->renderArea = { 0, 0, m_frameBufferWidth, m_frameBufferHeight };

		// Shaders
		ShaderResource configResource{};
		// Get shader resources for material shader
		if (!Resources.Load(BUILTIN_SHADER_NAME_MATERIAL, ResourceType::Shader, &configResource))
		{
			m_logger.Error("Init() - Failed to load builtin material shader config resource");
			return false;
		}

		// Create our material shader
		if (!Shaders.Create(&configResource.config))
		{
			m_logger.Error("Init() - Failed to create builtin material shader");
			return false;
		}

		// Unload resources for material shader config
		Resources.Unload(&configResource);
		// Obtain the shader id belonging to our material shader
		m_materialShaderId = Shaders.GetId(BUILTIN_SHADER_NAME_MATERIAL);

		// Get shader resources for ui shader
		if (!Resources.Load(BUILTIN_SHADER_NAME_UI, ResourceType::Shader, &configResource))
		{
			m_logger.Error("Init() - Failed to load builtin ui shader config resource");
			return false;
		}

		// Create our ui shader
		if (!Shaders.Create(&configResource.config))
		{
			m_logger.Error("Init() - Failed to create builtin material shader");
			return false;
		}

		// Unload resources for ui shader config
		Resources.Unload(&configResource);
		m_uiShaderId = Shaders.GetId(BUILTIN_SHADER_NAME_UI);

		const auto appState = application->GetState();
		const auto aspectRatio = static_cast<float>(appState->width) / static_cast<float>(appState->height);

		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		// UI projection and view
		m_uiProjection = glm::orthoRH_NO(0.0f, 1280.0f, 720.0f, 0.0f, -100.0f, 100.0f);

		m_logger.Info("Initialized Vulkan Renderer Backend");
		return true;
	}

	void RenderSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");

		Event.UnRegister(SystemEventCode::SetRenderMode, new EventCallback(this, &RenderSystem::OnEvent));

		for (u8 i = 0; i < m_windowRenderTargetCount; i++)
		{
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
				const auto fWidth = static_cast<f32>(m_frameBufferWidth);
				const auto fHeight = static_cast<f32>(m_frameBufferHeight);
				const auto aspectRatio = fWidth / fHeight;

				m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);
				m_uiProjection = glm::orthoRH_NO(0.0f, fWidth, fHeight, 0.0f, -100.0f, 100.0f);
				m_backend->OnResize(m_frameBufferWidth, m_frameBufferHeight);

				m_framesSinceResize = 0;
				m_resizing = false;
			}
			else
			{
				// Skip rendering this frame
				return true;
			}
		}

		// TODO: Views
		m_worldRenderPass->renderArea.z = m_frameBufferWidth;
		m_worldRenderPass->renderArea.w = m_frameBufferHeight;

		m_uiRenderPass->renderArea.z = m_frameBufferWidth;
		m_uiRenderPass->renderArea.w = m_frameBufferHeight;

		if (!m_activeWorldCamera)
		{
			// Get the default camera
			m_activeWorldCamera = Cam.GetDefault();
		}

		const auto view = m_activeWorldCamera->GetViewMatrix();
		const auto position = m_activeWorldCamera->GetPosition();

		if (m_backend->BeginFrame(packet->deltaTime))
		{
			const u8 attachmentIndex = m_backend->GetWindowAttachmentIndex();

			// Begin World RenderPass
			if (!m_backend->BeginRenderPass(m_worldRenderPass, &m_worldRenderPass->targets[attachmentIndex]))
			{
				m_logger.Error("DrawFrame() - BeginRenderPass(m_worldRenderPass) failed.");
				return false;
			}

			// Use our material shader for this
			if (!Shaders.UseById(m_materialShaderId))
			{
				m_logger.Error("DrawFrame() - Failed to use material shader");
				return false;
			}

			// Apply globals
			if (!Materials.ApplyGlobal(m_materialShaderId, &m_projection, &view, &m_ambientColor, &position, m_renderMode))
			{
				m_logger.Error("DrawFrame() - Failed to apply globals for material shader");
				return false;
			}

			// Draw all our World Geometry
			for (const auto& geometryData : packet->geometries)
			{
				Material* mat = geometryData.geometry->material;
				if (!mat) mat = Materials.GetDefault();

				// Apply our material if it hasn't already been updated this frame.
				// This prevents us from applying the same material multiple times
				const bool needsUpdate = mat->renderFrameNumber != m_backend->state.frameNumber;
				if (!Materials.ApplyInstance(mat, needsUpdate))
				{
					m_logger.Warn("DrawFrame() - Failed to apply material '{}'. Skipping draw", mat->name);
					continue;
				}
				mat->renderFrameNumber = static_cast<u32>(m_backend->state.frameNumber);

				// Apply locals
				Materials.ApplyLocal(mat, &geometryData.model);

				// Finally draw it
				m_backend->DrawGeometry(geometryData);
			}

			// End World RenderPass
			if (!m_backend->EndRenderPass(m_worldRenderPass))
			{
				m_logger.Error("DrawFrame() - EndRenderPass(m_worldRenderPass) failed");
				return false;
			}

			// Begin UI RenderPass
			if (!m_backend->BeginRenderPass(m_uiRenderPass, &m_uiRenderPass->targets[attachmentIndex]))
			{
				m_logger.Error("DrawFrame() - BeginRenderPass(BuiltinRenderPass::Ui) failed");
				return false;
			}

			// Use our ui shader for this
			if (!Shaders.UseById(m_uiShaderId))
			{
				m_logger.Error("DrawFrame() - Failed to use ui shader");
				return false;
			}

			// Apply globals
			if (!Materials.ApplyGlobal(m_uiShaderId, &m_uiProjection, &m_uiView, nullptr, nullptr, 0))
			{
				m_logger.Error("DrawFrame() - Failed to apply globals for ui shader");
				return false;
			}

			// Draw all our UI Geometry
			for (const auto& geometryData : packet->uiGeometries)
			{
				Material* mat = geometryData.geometry->material;
				if (!mat) mat = Materials.GetDefault();

				const bool needsUpdate = mat->renderFrameNumber != m_backend->state.frameNumber;
				// Apply our material
				if (!Materials.ApplyInstance(mat, needsUpdate))
				{
					m_logger.Warn("DrawFrame() - Failed to apply material '{}'. Skipping draw", mat->name);
					continue;
				}
				mat->renderFrameNumber = static_cast<u32>(m_backend->state.frameNumber);

				// Apply locals
				Materials.ApplyLocal(mat, &geometryData.model);

				// Finally draw it
				m_backend->DrawGeometry(geometryData);
			}

			// End UI RenderPass
			if (!m_backend->EndRenderPass(m_uiRenderPass))
			{
				m_logger.Error("DrawFrame() - EndRenderPass(m_uiRenderPass) failed");
				return false;
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

	RenderPass* RenderSystem::GetRenderPass(const char* name) const
	{
		return m_backend->GetRenderPass(name);
	}

	bool RenderSystem::CreateShader(Shader* shader, RenderPass* pass, const std::vector<char*>& stageFileNames, const std::vector<ShaderStage>& stages) const
	{
		return m_backend->CreateShader(shader, pass, stageFileNames, stages);
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

	void RenderSystem::RegenerateRenderTargets() const
	{
		// Create render targets of each. TODO: Should be configurable
		for (u8 i = 0; i < m_windowRenderTargetCount; i++)
		{
			m_backend->DestroyRenderTarget(&m_worldRenderPass->targets[i], false);
			m_backend->DestroyRenderTarget(&m_uiRenderPass->targets[i], false);

			Texture* windowTargetTexture = m_backend->GetWindowAttachment(i);
			Texture* depthTargetTexture = m_backend->GetDepthAttachment();

			// World render targets
			Texture* attachments[2] = { windowTargetTexture, depthTargetTexture };
			m_backend->CreateRenderTarget(2, attachments, m_worldRenderPass, m_frameBufferWidth, m_frameBufferHeight, &m_worldRenderPass->targets[i]);

			// UI render targets
			Texture* uiAttachments[1] = { windowTargetTexture };
			m_backend->CreateRenderTarget(1, uiAttachments, m_uiRenderPass, m_frameBufferWidth, m_frameBufferHeight, &m_uiRenderPass->targets[i]);
		}
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

	void RenderSystem::DestroyBackend() const
	{
		if (m_backend->type == RendererBackendType::Vulkan)
		{
			Memory.Free(m_backend, sizeof(RendererVulkan), MemoryType::RenderSystem);
		}
	}

	bool RenderSystem::OnEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code != SetRenderMode) return false;

		switch (const i32 mode = context.data.i32[0])
		{
			case RendererViewMode::Default:
				m_logger.Debug("Renderer mode set to default");
				m_renderMode = RendererViewMode::Default;
				break;
			case RendererViewMode::Lighting:
				m_logger.Debug("Renderer mode set to lighting");
				m_renderMode = RendererViewMode::Lighting;
				break;
			case RendererViewMode::Normals:
				m_logger.Debug("Renderer mode set to normals");
				m_renderMode = RendererViewMode::Normals;
				break;
		}
		return true;
	}
}
