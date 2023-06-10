
#pragma once
#include "console/console.h"
#include "defines.h"
#include "logger.h"
#include "renderer/renderer_types.h"
#include "renderer/render_view.h"

#include "systems/fonts/font_system.h"

int main(int argc, char** argv);

struct SDL_Window;
namespace C3D
{
	class Engine;
	struct EventContext;

	struct ApplicationConfig
	{
		String name;
		i32 x, y;
		i32 width, height;

		RendererPlugin* rendererPlugin;

		FontSystemConfig fontConfig;
		DynamicArray<RenderViewConfig> renderViews;
	};

	struct GameFrameData
	{
		DynamicArray<GeometryRenderData, LinearAllocator> worldGeometries;
	};

	struct EngineState
	{
		String name;

		u32 width = 0;
		u32 height = 0;

		bool running = false;
		bool suspended = false;
		bool initialized = false;

		f64 lastTime = 0;
	};

	class C3D_API Engine
	{
	public:
		explicit Engine(const ApplicationConfig& config);

		Engine(const Engine&) = delete;
		Engine(Engine&&) = delete;

		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) = delete;

		virtual ~Engine();

		void Init();

		void Run();
		void Quit();

		template<class SystemType>
		[[nodiscard]] SystemType& GetSystem(const u16 type) const
		{
			return m_systemsManager.GetSystem<SystemType>(type);
		}

		void SetBaseConsole(UIConsole* console)
		{
			m_console = console;
		}

		[[nodiscard]] UIConsole& GetBaseConsole() const
		{
			return *m_console;
		}

		virtual bool OnBoot()	{ return true; }
		virtual bool OnCreate() { return true; }

		virtual void OnUpdate(const f64 deltaTime)
		{
			m_console->OnUpdate();
		}

		virtual bool OnRender(RenderPacket& packet, f64 deltaTime)
		{
			return true;
		}

		virtual void AfterRender() {}

		virtual void OnResize(u16 width, u16 height) {}

		virtual void OnShutdown() {}

		void GetFrameBufferSize(u32* width, u32* height) const;

		[[nodiscard]] SDL_Window* GetWindow() const;

		[[nodiscard]] const EngineState* GetState() const;

	protected:
		LoggerInstance<16> m_logger;
		LinearAllocator m_frameAllocator;

		GameFrameData m_frameData;

		ApplicationConfig m_config;
		EngineState m_state;

		SystemManager m_systemsManager;

		const Engine* m_engine;
		UIConsole* m_console;

	private:
		void Shutdown();
		void HandleSdlEvents();

		bool OnResizeEvent(u16 code, void* sender, const EventContext& context);
		bool OnMinimizeEvent(u16 code, void* sender, const EventContext& context);
		bool OnFocusGainedEvent(u16 code, void* sender, const EventContext& context);

		SDL_Window* m_window{ nullptr };
		
		friend int ::main(int argc, char** argv);
	};

	Engine* CreateApplication();
	void DestroyApplication(const Engine* app);
}