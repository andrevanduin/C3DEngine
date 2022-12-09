
#pragma once
#include "defines.h"
#include "logger.h"
#include "renderer/renderer_types.h"
#include "renderer/render_view.h"

#include "systems/font_system.h"

int main(int argc, char** argv);

struct SDL_Window;
namespace C3D
{
	class Application;
	struct EventContext;

	struct ApplicationConfig
	{
		String name;
		i32 x, y;
		i32 width, height;

		FontSystemConfig fontConfig;
		DynamicArray<RenderViewConfig> renderViews;
	};

	struct ApplicationState
	{
		String name;

		u32 width = 0;
		u32 height = 0;

		bool running = false;
		bool suspended = false;
		bool initialized = false;

		f64 lastTime = 0;
	};

	class C3D_API Application
	{
	public:
		explicit Application(ApplicationConfig config);

		Application(const Application&) = delete;
		Application(Application&&) = delete;

		Application& operator=(const Application&) = delete;
		Application& operator=(Application&&) = delete;

		virtual ~Application();

		void Init();

		void Run();
		void Quit();

		virtual bool OnBoot()	{ return true; }
		virtual bool OnCreate() { return true; }

		virtual void OnUpdate(f64 deltaTime) {}
		virtual bool OnRender(RenderPacket& packet, f64 deltaTime) { return true; }

		virtual void OnResize(u16 width, u16 height) {}

		virtual void OnShutdown() {}

		void GetFrameBufferSize(u32* width, u32* height) const;

		[[nodiscard]] SDL_Window* GetWindow() const;

		[[nodiscard]] const ApplicationState* GetState() const;

	protected:
		LoggerInstance m_logger;
		LinearAllocator m_frameAllocator;

		ApplicationConfig m_config;
		ApplicationState m_state;

	private:
		void Shutdown();
		void HandleSdlEvents();

		bool OnResizeEvent(u16 code, void* sender, EventContext context);
		bool OnMinimizeEvent(u16 code, void* sender, EventContext context);
		bool OnFocusGainedEvent(u16 code, void* sender, EventContext context);

		SDL_Window* m_window{ nullptr };
		
		friend int ::main(int argc, char** argv);
	};

	Application* CreateApplication();
	void DestroyApplication(const Application* app);
}