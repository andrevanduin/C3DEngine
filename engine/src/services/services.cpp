
#include "services.h"

#include "core/application.h"
#include "core/input.h"
#include "core/events/event.h"

#include "core/logger.h"

namespace C3D
{
	LinearAllocator Services::m_allocator;

	// Systems
	InputSystem* Services::m_pInput		= nullptr;
	EventSystem* Services::m_pEvents	= nullptr;
	RenderSystem* Services::m_pRenderer = nullptr;

	bool Services::Init(Application* application)
	{
		Logger::PushPrefix("SERVICES");

		// 64 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = static_cast<u64>(64) * 1024 * 1024;
		m_allocator.Create(systemsAllocatorTotalSize);

		// Event System
		m_pEvents = m_allocator.New<EventSystem>();
		if (!m_pEvents->Init())
		{
			Logger::Fatal("Event System failed to be Initialized");
		}

		// Input System
		m_pInput = m_allocator.New<InputSystem>();
		if (!m_pInput->Init())
		{
			Logger::Fatal("Input System failed to be Initialized");
		}

		// Render System
		m_pRenderer = m_allocator.New<RenderSystem>();
		if (!m_pRenderer->Init(application))
		{
			Logger::Fatal("Render System failed to be Initialized");
		}

		Logger::PopPrefix();
		return true;
	}

	void Services::Shutdown()
	{
		Logger::PushPrefix("SERVICES");
		Logger::Info("Shutting down all services");

		m_pRenderer->Shutdown();
		m_pInput->Shutdown();
		m_pEvents->Shutdown();

		Logger::Info("Finished shutting down services. Destroying Allocator");
		m_allocator.Destroy();
		m_pInput = nullptr;
		Logger::Info("Shutdown finished");
		Logger::PopPrefix();
	}
}
