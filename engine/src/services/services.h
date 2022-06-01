
#pragma once
#include "core/defines.h"
#include "memory/linear_allocator.h"

// Systems
#include "core/input.h"
#include "core/events/event.h"

#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"

#define Input		C3D::Services::GetInput()
#define Event		C3D::Services::GetEvent()
#define Renderer	C3D::Services::GetRenderer()
#define Textures	C3D::Services::GetTextureSystem()
#define Materials	C3D::Services::GetMaterialSystem()
#define Geometric	C3D::Services::GetGeometrySystem()
#define Resources	C3D::Services::GetResourceSystem()

namespace C3D
{
	class Application;

	class C3D_API Services
	{
	public:
		static bool Init(Application* application, const ResourceSystemConfig& resourceSystemConfig, const TextureSystemConfig& textureSystemConfig,
			const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig);

		static void Shutdown();

		static InputSystem&		GetInput()			{ return *m_pInput;				}
		static EventSystem&		GetEvent()			{ return *m_pEvents;			}
		static RenderSystem&	GetRenderer()		{ return *m_pRenderer;			}
		static TextureSystem&	GetTextureSystem()	{ return *m_pTextureSystem;		}
		static MaterialSystem&	GetMaterialSystem()	{ return *m_pMaterialSystem;	}
		static GeometrySystem&	GetGeometrySystem()	{ return *m_pGeometrySystem;	}
		static ResourceSystem&	GetResourceSystem() { return *m_pResourceSystem;	}

	private:
		static EventSystem*		m_pEvents;
		static InputSystem*		m_pInput;
		static RenderSystem*	m_pRenderer;
		static TextureSystem*	m_pTextureSystem;
		static MaterialSystem*	m_pMaterialSystem;
		static GeometrySystem*	m_pGeometrySystem;
		static ResourceSystem*	m_pResourceSystem;

		static LinearAllocator m_allocator;
		static LoggerInstance m_logger;
	};
}

