
#pragma once
#include "containers/string.h"
#include "renderer/renderer_plugin.h"
#include "systems/fonts/font_system.h"

namespace C3D
{
	class Engine;

	struct ApplicationState
	{
		String name;

		i32 x, y;
		u32 width, height;

		RendererPlugin* rendererPlugin;
		FontSystemConfig fontConfig;

		DynamicArray<RenderViewConfig> renderViews;
	};

	class Application
	{
	public:
		explicit Application(ApplicationState* appState)
			: m_logger(appState->name.Data()), m_appState(appState), m_engine(nullptr)
		{}

		Application(const Application&) = delete;
		Application(Application&&) = delete;

		Application& operator=(const Application&) = delete;
		Application& operator=(Application&&) = delete;

		virtual ~Application() = default;

		virtual bool OnBoot() = 0;
		virtual bool OnRun() = 0;

		virtual void OnUpdate(f64 deltaTime) = 0;
		virtual bool OnRender(RenderPacket& packet, f64 deltaTime) = 0;

		virtual void OnResize() = 0;

		virtual void OnShutdown() = 0;

		virtual void OnLibraryLoad() = 0;
		virtual void OnLibraryUnload() = 0;

		friend Engine;

	protected:
		LoggerInstance<64> m_logger;

		ApplicationState* m_appState;
		const Engine* m_engine;
	};

	Application* CreateApplication();
	void InitApplication(Engine* engine);
	void DestroyApplication();
}
