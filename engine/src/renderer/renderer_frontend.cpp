
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"

namespace C3D
{
	RenderSystem::RenderSystem()
		: m_projection(), m_view(), m_nearClip(0.1f), m_farClip(1000.0f)
	{
	}

	bool RenderSystem::Init(Application* application)
	{
		Logger::PushPrefix("RENDERER");

		if (!CreateBackend(RendererBackendType::Vulkan))
		{
			Logger::Fatal("Failed to create Vulkan Renderer Backend");
			return false;
		}

		Logger::Info("Created Vulkan Renderer Backend");

		if (!m_backend->Init(application))
		{
			Logger::Fatal("Failed to Initialize Renderer Backend.");
			return false;
		}

		const auto appState = application->GetState();

		const auto aspectRatio = static_cast<float>(appState->width) / static_cast<float>(appState->height);

		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		Logger::Info("Initialized Vulkan Renderer Backend");
		Logger::PopPrefix();
		return true;
	}

	void RenderSystem::Shutdown() const
	{
		Logger::PrefixInfo("RENDERER", "Shutting Down");
		m_backend->Shutdown();
		DestroyBackend();
	}

	void RenderSystem::SetView(const mat4 view)
	{
		m_view = view;
	}

	void RenderSystem::OnResize(const u16 width, const u16 height)
	{
		const auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		m_projection = glm::perspectiveRH_NO(DegToRad(45.0f), aspectRatio, m_nearClip, m_farClip);

		m_backend->OnResize(width, height);
	}

	bool RenderSystem::DrawFrame(const RenderPacket* packet) const
	{
		if (!BeginFrame(packet->deltaTime)) return true;

		m_backend->UpdateGlobalState(m_projection, m_view, vec3(0.0f), vec4(1.0f), 0);

		static f32 angle = 0.0f;
		angle += 0.001f;

		mat4 model = glm::translate(vec3(0, 0, 0));
		model = glm::rotate(model, angle, vec3(0, 0, -1.0f));

		m_backend->UpdateObject(model);

		if (!EndFrame(packet->deltaTime))
		{
			Logger::Error("RenderSystem::EndFrame() failed.");
			return false;
		}
		return true;
	}

	bool RenderSystem::BeginFrame(const f32 deltaTime) const
	{
		return m_backend->BeginFrame(deltaTime);
	}

	bool RenderSystem::EndFrame(const f32 deltaTime) const
	{
		const bool result = m_backend->EndFrame(deltaTime);
		m_backend->state.frameNumber++;
		return result;
	}

	bool RenderSystem::CreateBackend(const RendererBackendType type)
	{
		if (type == RendererBackendType::Vulkan)
		{
			m_backend = Memory::New<RendererVulkan>(MemoryType::Renderer);
			return true;
		}

		Logger::Error("Tried Creating an Unsupported Renderer Backend: {}", ToUnderlying(type));
		return false;
	}

	void RenderSystem::DestroyBackend() const
	{
		if (m_backend->type == RendererBackendType::Vulkan)
		{
			Memory::Free(m_backend, sizeof(RendererVulkan), MemoryType::Renderer);
		}
	}
}
