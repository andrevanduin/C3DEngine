
#pragma once
#include "core/defines.h"
#include "memory/allocators/linear_allocator.h"

#define Systems		C3D::SystemManager::GetInstance()
#define Input		Systems.GetInputSystem()
#define Event		Systems.GetEventSystem()
#define Renderer	Systems.GetRenderSystem()
#define Textures	Systems.GetTextureSystem()
#define Materials	Systems.GetMaterialSystem()
#define Geometric	Systems.GetGeometrySystem()
#define Resources	Systems.GetResourceSystem()
#define Shaders		Systems.GetShaderSystem()
#define Cam			Systems.GetCameraSystem()
#define Views		Systems.GetRenderViewSystem()
#define Jobs		Systems.GetJobSystem()
#define Fonts		Systems.GetFontSystem()

namespace C3D
{
	class Engine;

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

	class C3D_API SystemManager
	{
	public:
		SystemManager();

		void InitBeforeBoot(const Engine* application, const ResourceSystemConfig& resourceSystemConfig, const ShaderSystemConfig& shaderSystemConfig);

		void InitAfterBoot(const JobSystemConfig& jobSystemConfig, const TextureSystemConfig& textureSystemConfig, const FontSystemConfig& fontSystemConfig,
			const CameraSystemConfig& cameraSystemConfig, const RenderViewSystemConfig& renderViewSystemConfig);

		void FinalInit(const MaterialSystemConfig& materialSystemConfig, const GeometrySystemConfig& geometrySystemConfig);

		void Shutdown();

		[[nodiscard]] InputSystem&			GetInputSystem()		const;
		[[nodiscard]] EventSystem&			GetEventSystem()		const;
		[[nodiscard]] RenderSystem&			GetRenderSystem()		const;
		[[nodiscard]] TextureSystem&		GetTextureSystem()		const;
		[[nodiscard]] MaterialSystem&		GetMaterialSystem()		const;
		[[nodiscard]] GeometrySystem&		GetGeometrySystem()		const;
		[[nodiscard]] ResourceSystem&		GetResourceSystem()		const;
		[[nodiscard]] ShaderSystem&			GetShaderSystem()		const;
		[[nodiscard]] CameraSystem&			GetCameraSystem()		const;
		[[nodiscard]] RenderViewSystem&		GetRenderViewSystem()	const;
		[[nodiscard]] JobSystem&			GetJobSystem()			const;
		[[nodiscard]] FontSystem&			GetFontSystem()			const;

		static SystemManager& GetInstance();

	private:
		template <class System>
		void ShutdownSystem(System* system);

		InputSystem*		m_pInputSystem;
		EventSystem*		m_pEventSystem;
		RenderSystem*		m_pRenderSystem;
		TextureSystem*		m_pTextureSystem;
		MaterialSystem*		m_pMaterialSystem;
		GeometrySystem*		m_pGeometrySystem;
		ResourceSystem*		m_pResourceSystem;
		ShaderSystem*		m_pShaderSystem;
		CameraSystem*		m_pCameraSystem;
		RenderViewSystem*	m_pRenderViewSystem;
		JobSystem*			m_pJobSystem;
		FontSystem*			m_pFontSystem;

		LinearAllocator		m_allocator;
		LoggerInstance<16>	m_logger;
	};

	template <class System>
	void SystemManager::ShutdownSystem(System* system)
	{
		system->Shutdown();
		m_allocator.Delete(MemoryType::CoreSystem, system);
	}
}

