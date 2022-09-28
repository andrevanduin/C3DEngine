
#pragma once
#include "core/defines.h"
#include "memory/linear_allocator.h"

#define Memory		C3D::Services::GetMemory()
#define Input		C3D::Services::GetInput()
#define Event		C3D::Services::GetEvent()
#define Renderer	C3D::Services::GetRenderer()
#define Textures	C3D::Services::GetTextureSystem()
#define Materials	C3D::Services::GetMaterialSystem()
#define Geometric	C3D::Services::GetGeometrySystem()
#define Resources	C3D::Services::GetResourceSystem()
#define Shaders		C3D::Services::GetShaderSystem()
#define Cam			C3D::Services::GetCameraSystem()

namespace C3D
{
	class Application;

	class MemorySystem;
	struct MemorySystemConfig;

	class InputSystem;
	struct ResourceSystemConfig;

	class EventSystem;
	struct ShaderSystemConfig;

	class RenderSystem;

	class TextureSystem;
	struct TextureSystemConfig;

	class MaterialSystem;
	struct MaterialSystemConfig;

	class GeometrySystem;
	struct GeometrySystemConfig;

	class ResourceSystem;
	class ShaderSystem;

	class CameraSystem;
	struct CameraSystemConfig;

	class C3D_API Services
	{
	public:
		static bool Init(Application* application, const MemorySystemConfig& memorySystemConfig, const ResourceSystemConfig& resourceSystemConfig,
			const ShaderSystemConfig& shaderSystemConfig, const TextureSystemConfig& textureSystemConfig, const MaterialSystemConfig& materialSystemConfig,
			const GeometrySystemConfig& geometrySystemConfig, const CameraSystemConfig& cameraSystemConfig);

		static bool InitMemory(const MemorySystemConfig& memorySystemConfig);

		static void Shutdown();

		static MemorySystem&	GetMemory()			{ return *m_pMemorySystem;		}
		static InputSystem&		GetInput()			{ return *m_pInputSystem;		}
		static EventSystem&		GetEvent()			{ return *m_pEventSystem;		}
		static RenderSystem&	GetRenderer()		{ return *m_pRenderSystem;		}
		static TextureSystem&	GetTextureSystem()	{ return *m_pTextureSystem;		}
		static MaterialSystem&	GetMaterialSystem()	{ return *m_pMaterialSystem;	}
		static GeometrySystem&	GetGeometrySystem()	{ return *m_pGeometrySystem;	}
		static ResourceSystem&	GetResourceSystem() { return *m_pResourceSystem;	}
		static ShaderSystem&	GetShaderSystem()	{ return *m_pShaderSystem;		}
		static CameraSystem&	GetCameraSystem()	{ return *m_pCameraSystem;		}

	private:
		static MemorySystem*	m_pMemorySystem;
		static EventSystem*		m_pEventSystem;
		static InputSystem*		m_pInputSystem;
		static RenderSystem*	m_pRenderSystem;
		static TextureSystem*	m_pTextureSystem;
		static MaterialSystem*	m_pMaterialSystem;
		static GeometrySystem*	m_pGeometrySystem;
		static ResourceSystem*	m_pResourceSystem;
		static ShaderSystem*	m_pShaderSystem;
		static CameraSystem*	m_pCameraSystem;

		static LinearAllocator m_allocator;
		static LoggerInstance m_logger;
	};
}

