
#include "services.h"

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
	LinearAllocator Services::m_allocator;
	LoggerInstance  Services::m_logger("SERVICES");

	// Systems
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
	JobSystem*			Services::m_pJobSystem		= nullptr;
	FontSystem*			Services::m_pFontSystem		= nullptr;

	void Services::InitBeforeBoot(const Engine* application, const ResourceSystemConfig& resourceSystemConfig, const ShaderSystemConfig& shaderSystemConfig)
	{
		// 32 mb of total space for all our systems
		constexpr u64 systemsAllocatorTotalSize = MebiBytes(32);
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

	void Services::InitAfterBoot(const JobSystemConfig& jobSystemConfig, const TextureSystemConfig& textureSystemConfig, 
		const FontSystemConfig& fontSystemConfig, const CameraSystemConfig& cameraSystemConfig, const RenderViewSystemConfig& renderViewSystemConfig)
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
		m_pViewSystem = m_allocator.New<RenderViewSystem>(MemoryType::CoreSystem);
		if (!m_pViewSystem->Init(renderViewSystemConfig))
		{
			m_logger.Fatal("ViewSystem failed to be Initialized.");
		}
	}

	void Services::FinalInit(const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig)
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

	void Services::Shutdown()
	{
		m_logger.Info("Shutting down all services");

		ShutdownSystem(m_pFontSystem);
		ShutdownSystem(m_pViewSystem);
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
}
