
#pragma once
#include "application.h"
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
	class Application;
	struct EventContext;

	struct GameFrameData
	{
		DynamicArray<GeometryRenderData, LinearAllocator> worldGeometries;
	};

	struct EngineState
	{
		String name;

		bool running = false;
		bool suspended = false;
		bool initialized = false;

		f64 lastTime = 0;
	};

	class C3D_API Engine final
	{
	public:
		explicit Engine(Application* application);

		void Init();

		void Run();
		void Quit();

		[[nodiscard]] bool OnBoot() const;

		void OnUpdate(f64 deltaTime) const;

		bool OnRender(RenderPacket& packet, f64 deltaTime) const;

		void OnResize(u32 width, u32 height) const;

		void GetFrameBufferSize(u32* width, u32* height) const;

		[[nodiscard]] SDL_Window* GetWindow() const;

		[[nodiscard]] const EngineState* GetState() const;

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

		void OnApplicationLibraryReload(Application* app);

	protected:
		LoggerInstance<16> m_logger;

		EngineState m_state;

		SystemManager m_systemsManager;

		const Engine* m_engine;
		UIConsole* m_console;

		Application* m_application;

	private:
		void Shutdown();
		void HandleSdlEvents();

		bool OnResizeEvent(u16 code, void* sender, const EventContext& context) const;
		bool OnMinimizeEvent(u16 code, void* sender, const EventContext& context);
		bool OnFocusGainedEvent(u16 code, void* sender, const EventContext& context);

		SDL_Window* m_window{ nullptr };
		
		friend int ::main(int argc, char** argv);
	};
}