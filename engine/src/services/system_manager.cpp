
#include "system_manager.h"

#include "core/engine.h"
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
#include "systems/font_system.h"
#include "systems/render_view_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	SystemManager::SystemManager()
		: m_pInputSystem(nullptr), m_pEventSystem(nullptr), m_pRenderSystem(nullptr), m_pTextureSystem(nullptr),
	      m_pMaterialSystem(nullptr), m_pGeometrySystem(nullptr), m_pResourceSystem(nullptr), m_pShaderSystem(nullptr),
		  m_pCameraSystem(nullptr), m_pRenderViewSystem(nullptr), m_pJobSystem(nullptr), m_pFontSystem(nullptr),
	      m_logger("SERVICES")
	{}

	void SystemManager::InitBeforeBoot(const Engine* application, const ResourceSystemConfig& resourceSystemConfig, const ShaderSystemConfig& shaderSystemConfig)
	{
		// 8 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = MebiBytes(8);
		m_allocator.Create("LINEAR_SYSTEM_ALLOCATOR", systemsAllocatorTotalSize);

		// Event System
		m_pEventSystem = m_allocator.New<EventSystem>(MemoryType::CoreSystem);
		if (!m_pEventSystem->Init())
		{
			m_logger.Fatal("EventSystem failed to be Initialized.");
		}

		// Input System
		m_pInputSystem = m_allocator.New<InputSystem>(MemoryType::CoreSystem);
		if (!m_pInputSystem->Init())
		{
			m_logger.Fatal("InputSystem failed to be Initialized.");
		}

		// Resource System
		m_pResourceSystem = m_allocator.New<ResourceSystem>(MemoryType::CoreSystem);
		if (!m_pResourceSystem->Init(resourceSystemConfig))
		{
			m_logger.Fatal("ResourceSystem failed to be Initialized.");
		}

		// Shader System
		m_pShaderSystem = m_allocator.New<ShaderSystem>(MemoryType::CoreSystem);
		if (!m_pShaderSystem->Init(shaderSystemConfig))
		{
			m_logger.Fatal("ShaderSystem failed to be Initialized.");
		}

		// Render System
		m_pRenderSystem = m_allocator.New<RenderSystem>(MemoryType::CoreSystem);
		if (!m_pRenderSystem->Init(application))
		{
			m_logger.Fatal("RenderSystem failed to be Initialized.");
		}
	}

	void SystemManager::InitAfterBoot(const JobSystemConfig& jobSystemConfig, const TextureSystemConfig& textureSystemConfig,
		const FontSystemConfig& fontSystemConfig, const CameraSystemConfig& cameraSystemConfig, 
		const RenderViewSystemConfig& renderViewSystemConfig)
	{
		// Job System
		m_pJobSystem = m_allocator.New<JobSystem>(MemoryType::CoreSystem);
		if (!m_pJobSystem->Init(jobSystemConfig))
		{
			m_logger.Fatal("JobSystem failed to be Initialized.");
		}

		// Texture System
		m_pTextureSystem = m_allocator.New<TextureSystem>(MemoryType::CoreSystem);
		if (!m_pTextureSystem->Init(textureSystemConfig))
		{
			m_logger.Fatal("TextureSystem failed to be Initialized.");
		}

		// Font System
		m_pFontSystem = m_allocator.New<FontSystem>(MemoryType::CoreSystem);
		if (!m_pFontSystem->Init(fontSystemConfig))
		{
			m_logger.Fatal("FontSystem failed to be Initialized.");
		}

		// Camera System
		m_pCameraSystem = m_allocator.New<CameraSystem>(MemoryType::CoreSystem);
		if (!m_pCameraSystem->Init(cameraSystemConfig))
		{
			m_logger.Fatal("CameraSystem failed to be Initialized.");
		}

		// View System
		m_pRenderViewSystem = m_allocator.New<RenderViewSystem>(MemoryType::CoreSystem);
		if (!m_pRenderViewSystem->Init(renderViewSystemConfig))
		{
			m_logger.Fatal("ViewSystem failed to be Initialized.");
		}
	}

	void SystemManager::FinalInit(const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig)
	{
		// Material System
		m_pMaterialSystem = m_allocator.New<MaterialSystem>(MemoryType::CoreSystem);
		if (!m_pMaterialSystem->Init(materialSystemConfig))
		{
			m_logger.Fatal("Material System failed to be Initialized.");
		}

		// Geometry System
		m_pGeometrySystem = m_allocator.New<GeometrySystem>(MemoryType::CoreSystem);
		if (!m_pGeometrySystem->Init(geometrySystemConfig))
		{
			m_logger.Fatal("GeometrySystem failed to be Initialized.");
		}
	}

	void SystemManager::Shutdown()
	{
		m_logger.Info("Shutting down all services");

		ShutdownSystem(m_pFontSystem);
		ShutdownSystem(m_pRenderViewSystem);
		ShutdownSystem(m_pCameraSystem);
		ShutdownSystem(m_pGeometrySystem);
		ShutdownSystem(m_pMaterialSystem);
		ShutdownSystem(m_pTextureSystem);
		ShutdownSystem(m_pShaderSystem);
		ShutdownSystem(m_pRenderSystem);
		ShutdownSystem(m_pResourceSystem);
		ShutdownSystem(m_pInputSystem);
		ShutdownSystem(m_pEventSystem);
		ShutdownSystem(m_pJobSystem);

		m_logger.Info("Destroying Linear Allocator");
		m_allocator.Destroy();
		m_logger.Info("Shutdown finished");
	}

	InputSystem& SystemManager::GetInputSystem() const
	{
		return *m_pInputSystem;
	}

	EventSystem& SystemManager::GetEventSystem() const
	{
		return *m_pEventSystem;
	}

	RenderSystem& SystemManager::GetRenderSystem() const
	{
		return *m_pRenderSystem;
	}

	TextureSystem& SystemManager::GetTextureSystem() const
	{
		return *m_pTextureSystem;
	}

	MaterialSystem& SystemManager::GetMaterialSystem() const
	{
		return *m_pMaterialSystem;
	}

	GeometrySystem& SystemManager::GetGeometrySystem() const
	{
		return *m_pGeometrySystem;
	}

	ResourceSystem& SystemManager::GetResourceSystem() const
	{
		return *m_pResourceSystem;
	}

	ShaderSystem& SystemManager::GetShaderSystem() const
	{
		return *m_pShaderSystem;
	}

	CameraSystem& SystemManager::GetCameraSystem() const
	{
		return *m_pCameraSystem;
	}

	RenderViewSystem& SystemManager::GetRenderViewSystem() const
	{
		return *m_pRenderViewSystem;
	}

	JobSystem& SystemManager::GetJobSystem() const
	{
		return *m_pJobSystem;
	}

	FontSystem& SystemManager::GetFontSystem() const
	{
		return *m_pFontSystem;
	}

	SystemManager& SystemManager::GetInstance()
	{
		static auto services = new SystemManager();
		return *services;
	}
}
