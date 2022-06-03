
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"
#include "core/c3d_string.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"
#include "services/services.h"

namespace C3D
{
	RenderSystem::RenderSystem()
		: m_logger("RENDERER"), m_projection(), m_view(), m_uiProjection(), m_uiView(), m_nearClip(0.1f), m_farClip(1000.0f)
	{}

	bool RenderSystem::Init(Application* application)
	{
		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			m_logger.Fatal("Failed to create Vulkan Renderer Backend");
			return false;
		}

		m_logger.Info("Created Vulkan Renderer Backend");

		if (!m_backend->Init(application))
		{
			m_logger.Fatal("Failed to Initialize Renderer Backend.");
			return false;
		}

		const auto appState = application->GetState();

		const auto aspectRatio = static_cast<float>(appState->width) / static_cast<float>(appState->height);

		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		// TODO: Make camera starting position configurable
		m_view = translate(mat4(1.0f), vec3(0, 0, -30.0f));
		m_view = inverse(m_view);

		// UI projection and view
		m_uiProjection = glm::orthoRH_NO(0.0f, 1280.0f, 720.0f, 0.0f, -100.0f, 100.0f);
		m_uiView = inverse(mat4(1.0f));

		m_logger.Info("Initialized Vulkan Renderer Backend");
		return true;
	}

	void RenderSystem::Shutdown() const
	{
		m_logger.Info("Shutting Down");

		m_backend->Shutdown();
		DestroyBackend();
	}

	void RenderSystem::SetView(const mat4 view)
	{
		m_view = view;
	}

	void RenderSystem::OnResize(const u16 width, const u16 height)
	{
		const auto fWidth = static_cast<float>(width);
		const auto fHeight = static_cast<float>(height);
		const auto aspectRatio = fWidth / fHeight;

		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);
		m_uiProjection = glm::orthoRH_NO(0.0f, fWidth, fHeight, 0.0f, -100.0f, 100.0f);

		m_backend->OnResize(width, height);
	}

	bool RenderSystem::DrawFrame(const RenderPacket* packet) const
	{
		if (!m_backend->BeginFrame(packet->deltaTime)) return true;

		// Begin World RenderPass
		if (!m_backend->BeginRenderPass(BuiltinRenderPass::World))
		{
			m_logger.Error("BeginRenderPass() BuiltinRenderPass::World failed");
			return false;
		}

		m_backend->UpdateGlobalWorldState(m_projection, m_view, vec3(0.0f), vec4(1.0f), 0);

		// Draw all our World Geometry
		const u32 count = packet->geometryCount;
		for (u32 i = 0; i < count; i++)
		{
			m_backend->DrawGeometry(packet->geometries[i]);
		}

		// End World RenderPass
		if (!m_backend->EndRenderPass(BuiltinRenderPass::World))
		{
			m_logger.Error("EndRenderPass() BuiltinRenderPass::World failed");
			return false;
		}

		// Begin UI RenderPass
		if (!m_backend->BeginRenderPass(BuiltinRenderPass::Ui))
		{
			m_logger.Error("BeginRenderPass() BuiltinRenderPass::Ui failed");
			return false;
		}

		m_backend->UpdateGlobalUiState(m_uiProjection, m_uiView, 0);

		// Draw all our UI Geometry
		const u32 uiCount = packet->uiGeometryCount;
		for (u32 i = 0; i < uiCount; i++)
		{
			m_backend->DrawGeometry(packet->uiGeometries[i]);
		}

		// End UI RenderPass
		if (!m_backend->EndRenderPass(BuiltinRenderPass::Ui))
		{
			m_logger.Error("EndRenderPass() BuiltinRenderPass::Ui failed");
			return false;
		}

		// End frame
		if (!m_backend->EndFrame(packet->deltaTime))
		{
			m_logger.Error("EndFrame() failed");
			return false;
		}
		return true;
	}

	void RenderSystem::CreateTexture(const u8* pixels, Texture* texture) const
	{
		return m_backend->CreateTexture(pixels, texture);
	}

	bool RenderSystem::CreateGeometry(Geometry* geometry, const u32 vertexSize, const u32 vertexCount, const void* vertices,
		const u32 indexSize, const u32 indexCount, const void* indices) const
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

	bool RenderSystem::GetRenderPassId(const char* name, u8* outRenderPassId) const
	{
		// HACK: Need dynamic RenderPasses instead of hardcoding them.
		if (IEquals("RenderPass.Builtin.World", name)) {
			*outRenderPassId = BuiltinRenderPass::World;
			return true;
		}
		if (IEquals("RenderPass.Builtin.UI", name)) {
			*outRenderPassId = BuiltinRenderPass::Ui;
			return true;
		}

		m_logger.Error("GetRenderPassId() - No RenderPass with name '{}' exists", name);
		*outRenderPassId = INVALID_ID_U8;
		return false;
	}

	bool RenderSystem::CreateShader(Shader* shader, u8 renderPassId, u8 stageCount, char** stageFileNames, ShaderStage* stages)
	{
		return m_backend->CreateShader(shader, renderPassId, stageCount, stageFileNames, stages);
	}

	void RenderSystem::DestroyShader(Shader* shader)
	{
		return m_backend->DestroyShader(shader);
	}

	bool RenderSystem::InitializeShader(Shader* shader)
	{
		return m_backend->InitializeShader(shader);
	}

	bool RenderSystem::UseShader(Shader* shader)
	{
		return m_backend->UseShader(shader);
	}

	bool RenderSystem::ShaderBindGlobals(Shader* shader)
	{
		return m_backend->ShaderBindGlobals(shader);
	}

	bool RenderSystem::ShaderBindInstance(Shader* shader, u32 instanceId)
	{
		return m_backend->ShaderBindInstance(shader, instanceId);
	}

	bool RenderSystem::ShaderApplyGlobals(Shader* shader)
	{
		return m_backend->ShaderApplyGlobals(shader);
	}

	bool RenderSystem::ShaderApplyInstance(Shader* shader)
	{
		return m_backend->ShaderApplyInstance(shader);
	}

	bool RenderSystem::AcquireShaderInstanceResources(Shader* shader, u32* outInstanceId)
	{
		return m_backend->AcquireShaderInstanceResources(shader, outInstanceId);
	}

	bool RenderSystem::ReleaseShaderInstanceResources(Shader* shader, u32 instanceId)
	{
		return m_backend->ReleaseShaderInstanceResources(shader, instanceId);
	}

	bool RenderSystem::SetUniform(Shader* shader, ShaderUniform* uniform, const void* value)
	{
		return m_backend->SetUniform(shader, uniform, value);
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
}
