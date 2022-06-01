
#include "services.h"

#include "core/application.h"
#include "core/input.h"
#include "core/events/event.h"

#include "core/logger.h"

namespace C3D
{
	LinearAllocator Services::m_allocator;
	LoggerInstance  Services::m_logger("SERVICES");

	// Systems
	InputSystem*	Services::m_pInput			= nullptr;
	EventSystem*	Services::m_pEvents			= nullptr;
	RenderSystem*	Services::m_pRenderer		= nullptr;
	ResourceSystem* Services::m_pResourceSystem = nullptr;
	TextureSystem*	Services::m_pTextureSystem	= nullptr;
	MaterialSystem* Services::m_pMaterialSystem = nullptr;
	GeometrySystem* Services::m_pGeometrySystem = nullptr;

	bool Services::Init(Application* application, const ResourceSystemConfig& resourceSystemConfig, const TextureSystemConfig& textureSystemConfig, 
		const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig)
	{
		// 64 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = static_cast<u64>(64) * 1024 * 1024;
		m_allocator.Create(systemsAllocatorTotalSize);

		// Event System
		m_pEvents = m_allocator.New<EventSystem>();
		if (!m_pEvents->Init())
		{
			m_logger.Fatal("GetEvent System failed to be Initialized");
		}

		// Input System
		m_pInput = m_allocator.New<InputSystem>();
		if (!m_pInput->Init())
		{
			m_logger.Fatal("GetInput System failed to be Initialized");
		}

		// Resource System
		m_pResourceSystem = m_allocator.New<ResourceSystem>();
		if (!m_pResourceSystem->Init(resourceSystemConfig))
		{
			m_logger.Fatal("Resource System failed to be Initialized");
		}

		// Render System
		m_pRenderer = m_allocator.New<RenderSystem>();
		if (!m_pRenderer->Init(application))
		{
			m_logger.Fatal("Render System failed to be Initialized");
		}

		// Texture System
		m_pTextureSystem = m_allocator.New<TextureSystem>();
		if (!m_pTextureSystem->Init(textureSystemConfig))
		{
			m_logger.Fatal("Texture System failed to be Initialized");
		}

		// Material System
		m_pMaterialSystem = m_allocator.New<MaterialSystem>();
		if (!m_pMaterialSystem->Init(materialSystemConfig))
		{
			m_logger.Fatal("Material System failed to be Initialized");
		}

		// Geometry System
		m_pGeometrySystem = m_allocator.New<GeometrySystem>();
		if (!m_pGeometrySystem->Init(geometrySystemConfig))
		{
			m_logger.Fatal("Geometry System failed to be Initialized");
		}
		return true;
	}

	void Services::Shutdown()
	{
		m_logger.Info("Shutting down all services");

		m_pGeometrySystem->Shutdown();
		m_pMaterialSystem->Shutdown();
		m_pTextureSystem->Shutdown();
		m_pResourceSystem->Shutdown();
		m_pRenderer->Shutdown();
		m_pInput->Shutdown();
		m_pEvents->Shutdown();

		m_logger.Info("Finished shutting down services. Destroying Allocator");
		m_allocator.Destroy();
		m_pInput = nullptr;
		m_logger.Info("Shutdown finished");
	}
}
