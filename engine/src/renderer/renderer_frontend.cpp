
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"

#include "renderer/vulkan/renderer_vulkan.h"

namespace C3D
{
	RendererBackend* Renderer::m_backend = nullptr;

	bool Renderer::Init(Application* application)
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

		Logger::Info("Initialized Vulkan Renderer Backend");
		Logger::PopPrefix();
		return true;
	}

	void Renderer::Shutdown()
	{
		Logger::PrefixInfo("RENDERER", "Shutting Down");
		m_backend->Shutdown();
		DestroyBackend();
	}

	void Renderer::OnResize(const u16 width, const u16 height)
	{
		m_backend->OnResize(width, height);
	}

	bool Renderer::DrawFrame(const RenderPacket* packet)
	{
		if (!BeginFrame(packet->deltaTime)) return true;


		if (!EndFrame(packet->deltaTime))
		{
			Logger::Error("Renderer::EndFrame() failed.");
			return false;
		}
		return true;
	}

	bool Renderer::BeginFrame(const f32 deltaTime)
	{
		return m_backend->BeginFrame(deltaTime);
	}

	bool Renderer::EndFrame(const f32 deltaTime)
	{
		const bool result = m_backend->EndFrame(deltaTime);
		m_backend->state.frameNumber++;
		return result;
	}

	bool Renderer::CreateBackend(const RendererBackendType type)
	{
		if (type == RendererBackendType::Vulkan)
		{
			m_backend = Memory::New<RendererVulkan>(MemoryType::Renderer);
			return true;
		}

		Logger::Error("Tried Creating an Unsupported Renderer Backend: {}", ToUnderlying(type));
		return false;
	}

	void Renderer::DestroyBackend()
	{
		if (m_backend->type == RendererBackendType::Vulkan)
		{
			Memory::Free(m_backend, sizeof(RendererVulkan), MemoryType::Renderer);
		}
	}
}
