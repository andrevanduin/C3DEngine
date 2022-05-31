
#include "services.h"

#include "core/application.h"
#include "core/input.h"
#include "core/events/event.h"

#include "core/logger.h"

namespace C3D
{
	LinearAllocator Services::m_allocator;

	// Systems
	InputSystem* Services::m_pInput				= nullptr;
	EventSystem* Services::m_pEvents			= nullptr;
	RenderSystem* Services::m_pRenderer			= nullptr;
	TextureSystem* Services::m_pTextureSystem	= nullptr;
	MaterialSystem* Services::m_pMaterialSystem = nullptr;
	GeometrySystem* Services::m_pGeometrySystem = nullptr;

	bool Services::Init(Application* application, const TextureSystemConfig& textureSystemConfig, const MaterialSystemConfig& materialSystemConfig, 
		const GeometrySystemConfig& geometrySystemConfig)
	{
		Logger::PushPrefix("SERVICES");

		// 64 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = static_cast<u64>(64) * 1024 * 1024;
		m_allocator.Create(systemsAllocatorTotalSize);

		// Event System
		m_pEvents = m_allocator.New<EventSystem>();
		if (!m_pEvents->Init())
		{
			Logger::Fatal("GetEvent System failed to be Initialized");
		}

		// Input System
		m_pInput = m_allocator.New<InputSystem>();
		if (!m_pInput->Init())
		{
			Logger::Fatal("GetInput System failed to be Initialized");
		}

		// Render System
		m_pRenderer = m_allocator.New<RenderSystem>();
		if (!m_pRenderer->Init(application))
		{
			Logger::Fatal("Render System failed to be Initialized");
		}

		m_pTextureSystem = m_allocator.New<TextureSystem>();
		if (!m_pTextureSystem->Init(textureSystemConfig))
		{
			Logger::Fatal("Texture System failed to be Initialized");
		}

		m_pMaterialSystem = m_allocator.New<MaterialSystem>();
		if (!m_pMaterialSystem->Init(materialSystemConfig))
		{
			Logger::Fatal("Material System failed to be Initialized");
		}

		m_pGeometrySystem = m_allocator.New<GeometrySystem>();
		if (!m_pGeometrySystem->Init(geometrySystemConfig))
		{
			
		}

		Logger::PopPrefix();
		return true;
	}

	void Services::Shutdown()
	{
		Logger::PushPrefix("SERVICES");
		Logger::Info("Shutting down all services");

		m_pMaterialSystem->Shutdown();
		m_pTextureSystem->Shutdown();
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
