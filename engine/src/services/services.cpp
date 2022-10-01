
#include "services.h"

#include "core/application.h"
#include "core/input.h"
#include "core/events/event.h"
#include "core/logger.h"

#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "systems/render_view_system.h"

namespace C3D
{
	LinearAllocator Services::m_allocator;
	LoggerInstance  Services::m_logger("SERVICES");

	// Systems
	MemorySystem*		Services::m_pMemorySystem	= nullptr;
	InputSystem*		Services::m_pInputSystem	= nullptr;
	EventSystem*		Services::m_pEventSystem	= nullptr;
	RenderSystem*		Services::m_pRenderSystem	= nullptr;
	ResourceSystem*		Services::m_pResourceSystem = nullptr;
	TextureSystem*		Services::m_pTextureSystem	= nullptr;
	MaterialSystem*		Services::m_pMaterialSystem = nullptr;
	GeometrySystem*		Services::m_pGeometrySystem = nullptr;
	ShaderSystem*		Services::m_pShaderSystem	= nullptr;
	CameraSystem*		Services::m_pCameraSystem	= nullptr;
	RenderViewSystem*	Services::m_pViewSystem		= nullptr;

	bool Services::Init(const Application* application, const MemorySystemConfig& memorySystemConfig, const ResourceSystemConfig& resourceSystemConfig,
	                    const ShaderSystemConfig& shaderSystemConfig, const TextureSystemConfig& textureSystemConfig, const MaterialSystemConfig& materialSystemConfig, 
	                    const GeometrySystemConfig& geometrySystemConfig, const CameraSystemConfig& cameraSystemConfig, const RenderViewSystemConfig& viewSystemConfig)
	{
		m_pMemorySystem = new MemorySystem();
		if (!m_pMemorySystem->Init(memorySystemConfig))
		{
			m_logger.Fatal("MemorySystem failed to be Initialized");
		}

		// 64 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = static_cast<u64>(64) * 1024 * 1024;
		m_allocator.Create(systemsAllocatorTotalSize);

		// Event System
		m_pEventSystem = m_allocator.New<EventSystem>();
		if (!m_pEventSystem->Init())
		{
			m_logger.Fatal("EventSystem failed to be Initialized.");
		}

		// Input System
		m_pInputSystem = m_allocator.New<InputSystem>();
		if (!m_pInputSystem->Init())
		{
			m_logger.Fatal("InputSystem failed to be Initialized.");
		}

		// Resource System
		m_pResourceSystem = m_allocator.New<ResourceSystem>();
		if (!m_pResourceSystem->Init(resourceSystemConfig))
		{
			m_logger.Fatal("ResourceSystem failed to be Initialized.");
		}

		// Shader System
		m_pShaderSystem = m_allocator.New<ShaderSystem>();
		if (!m_pShaderSystem->Init(shaderSystemConfig))
		{
			m_logger.Fatal("ShaderSystem failed to be Initialized.");
		}

		// Render System
		m_pRenderSystem = m_allocator.New<RenderSystem>();
		if (!m_pRenderSystem->Init(application))
		{
			m_logger.Fatal("RenderSystem failed to be Initialized.");
		}

		// Texture System
		m_pTextureSystem = m_allocator.New<TextureSystem>();
		if (!m_pTextureSystem->Init(textureSystemConfig))
		{
			m_logger.Fatal("TextureSystem failed to be Initialized.");
		}

		// Material System
		m_pMaterialSystem = m_allocator.New<MaterialSystem>();
		if (!m_pMaterialSystem->Init(materialSystemConfig))
		{
			m_logger.Fatal("Material System failed to be Initialized.");
		}

		// Geometry System
		m_pGeometrySystem = m_allocator.New<GeometrySystem>();
		if (!m_pGeometrySystem->Init(geometrySystemConfig))
		{
			m_logger.Fatal("GeometrySystem failed to be Initialized.");
		}

		// Camera System
		m_pCameraSystem = m_allocator.New<CameraSystem>();
		if (!m_pCameraSystem->Init(cameraSystemConfig))
		{
			m_logger.Fatal("CameraSystem failed to be Initialized.");
		}

		// View System
		m_pViewSystem = m_allocator.New<RenderViewSystem>();
		if (!m_pViewSystem->Init(viewSystemConfig))
		{
			m_logger.Fatal("ViewSystem failed to be Initialized.");
		}

		return true;
	}

	bool Services::InitMemory(const MemorySystemConfig& memorySystemConfig)
	{
		m_pMemorySystem = new MemorySystem();
		if (!m_pMemorySystem->Init(memorySystemConfig))
		{
			m_logger.Fatal("MemorySystem failed to be Initialized");
		}
		return true;
	}

	void Services::Shutdown()
	{
		m_logger.Info("Shutting down all services");

		m_pViewSystem->Shutdown();
		m_pCameraSystem->Shutdown();
		m_pGeometrySystem->Shutdown();
		m_pMaterialSystem->Shutdown();
		m_pTextureSystem->Shutdown();
		m_pShaderSystem->Shutdown();
		m_pRenderSystem->Shutdown();
		m_pResourceSystem->Shutdown();
		m_pInputSystem->Shutdown();
		m_pEventSystem->Shutdown();

		m_logger.Info("Destroying Linear Allocator");
		m_allocator.Destroy();

		m_logger.Info("Shutting down MemorySystem");
		m_pMemorySystem->Shutdown();
		m_logger.Info("Shutdown finished");
	}
}
