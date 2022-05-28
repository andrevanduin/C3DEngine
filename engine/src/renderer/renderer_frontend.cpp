
#include "renderer_frontend.h"

#include "core/logger.h"
#include "core/memory.h"
#include "core/application.h"

#include "math/c3d_math.h"

#include "renderer/vulkan/renderer_vulkan.h"

namespace C3D
{
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

	void RenderSystem::OnResize(const u16 width, const u16 height) const
	{
		m_backend->OnResize(width, height);
	}

	bool RenderSystem::DrawFrame(const RenderPacket* packet) const
	{
		if (!BeginFrame(packet->deltaTime)) return true;

		const mat4 projection = glm::perspective(DegToRad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
		static f32 z = -10.0f, modifier;

		if (z >= -10.0f) modifier = -0.01f;
		if (z <= -50.0f) modifier = 0.01f;

		z += modifier;

		const mat4 view = glm::translate(vec3(0, 0, z)); // 30.0f

		m_backend->UpdateGlobalState(projection, view, vec3(0.0f), vec4(1.0f), 0);

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
