
#pragma once
#include "defines.h"

int C3D_API main(int argc, char** argv);

struct SDL_Window;
namespace C3D
{
	class Application;
	struct EventContext;

	struct ApplicationConfig
	{
		string name;
		i32 x, y;
		i32 width, height;
	};

	struct ApplicationState
	{
		string name;

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
		explicit Application(const ApplicationConfig& config);
		virtual ~Application();

		void Run();
		virtual void OnUpdate(f64 deltaTime) {}
		virtual void OnRender(f64 deltaTime) {}

		virtual void OnResize(u16 width, u16 height) {}

		void GetFrameBufferSize(u32* width, u32* height) const;

		[[nodiscard]] SDL_Window* GetWindow() const;

	private:
		ApplicationState m_state;

		void Shutdown();
		void HandleSdlEvents();

		bool OnResizeEvent(u16 code, void* sender, void* listener, EventContext context);
		bool OnMinimizeEvent(u16 code, void* sender, void* listener, EventContext context);
		bool OnFocusGainedEvent(u16 code, void* sender, void* listener, EventContext context);

		bool OnKeyEvent(u16 code, void* sender, void* listener, EventContext context);

		SDL_Window* m_window{ nullptr };

		friend int ::main(int argc, char** argv);
	};

	Application* CreateApplication();
}