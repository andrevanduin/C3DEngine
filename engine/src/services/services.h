
#pragma once
#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"

#define Input		C3D::Services::GetInput()
#define Event		C3D::Services::GetEvent()
#define Renderer	C3D::Services::GetRenderer()
#define Textures	C3D::Services::GetTextureSystem()
#define Materials	C3D::Services::GetMaterialSystem()
#define Geometric	C3D::Services::GetGeometrySystem()
#define Resources	C3D::Services::GetResourceSystem()
#define Shaders		C3D::Services::GetShaderSystem()
#define Cam			C3D::Services::GetCameraSystem()
#define Views		C3D::Services::GetViewSystem()
#define Jobs		C3D::Services::GetJobSystem()
#define Fonts		C3D::Services::GetFontSystem()

namespace C3D
{
	class Application;

	class EventSystem;
	class InputSystem;

	class ResourceSystem;
	struct ResourceSystemConfig;

	class ShaderSystem;
	struct ShaderSystemConfig;

	class RenderSystem;

	class JobSystem;
	struct JobSystemConfig;

	class TextureSystem;
	struct TextureSystemConfig;

	class FontSystem;
	struct FontSystemConfig;

	class CameraSystem;
	struct CameraSystemConfig;

	class RenderViewSystem;
	struct RenderViewSystemConfig;

	class MaterialSystem;
	struct MaterialSystemConfig;

	class GeometrySystem;
	struct GeometrySystemConfig;

	class C3D_API Services
	{
	public:
		static void InitBeforeBoot(const Application* application, const ResourceSystemConfig& resourceSystemConfig, const ShaderSystemConfig& shaderSystemConfig);

		static void InitAfterBoot(const JobSystemConfig& jobSystemConfig, const TextureSystemConfig& textureSystemConfig, const FontSystemConfig& fontSystemConfig,
			const CameraSystemConfig& cameraSystemConfig, const RenderViewSystemConfig& renderViewSystemConfig);

		static void FinalInit(const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig);

		static void Shutdown();

		static InputSystem&			GetInput()			{ return *m_pInputSystem;		}
		static EventSystem&			GetEvent()			{ return *m_pEventSystem;		}
		static RenderSystem&		GetRenderer()		{ return *m_pRenderSystem;		}
		static TextureSystem&		GetTextureSystem()	{ return *m_pTextureSystem;		}
		static MaterialSystem&		GetMaterialSystem()	{ return *m_pMaterialSystem;	}
		static GeometrySystem&		GetGeometrySystem()	{ return *m_pGeometrySystem;	}
		static ResourceSystem&		GetResourceSystem() { return *m_pResourceSystem;	}
		static ShaderSystem&		GetShaderSystem()	{ return *m_pShaderSystem;		}
		static CameraSystem&		GetCameraSystem()	{ return *m_pCameraSystem;		}
		static RenderViewSystem&	GetViewSystem()		{ return *m_pViewSystem;		}
		static JobSystem&			GetJobSystem()		{ return *m_pJobSystem;			}
		static FontSystem&			GetFontSystem()		{ return *m_pFontSystem;		}

	private:
		static EventSystem*			m_pEventSystem;
		static InputSystem*			m_pInputSystem;
		static RenderSystem*		m_pRenderSystem;
		static TextureSystem*		m_pTextureSystem;
		static MaterialSystem*		m_pMaterialSystem;
		static GeometrySystem*		m_pGeometrySystem;
		static ResourceSystem*		m_pResourceSystem;
		static ShaderSystem*		m_pShaderSystem;
		static CameraSystem*		m_pCameraSystem;
		static RenderViewSystem*	m_pViewSystem;
		static JobSystem*			m_pJobSystem;
		static FontSystem*			m_pFontSystem;

		static LinearAllocator m_allocator;
		static LoggerInstance m_logger;
	};
}

