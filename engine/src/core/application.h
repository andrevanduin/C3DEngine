
#pragma once
#include "defines.h"

int C3D_API main(int argc, char** argv);

struct SDL_Window;
namespace C3D
{
	struct ApplicationConfig
	{
		string name;
		i32 x, y;
		i32 width, height;
	};

	class C3D_API Application
	{
	public:
		explicit Application(const ApplicationConfig& config);
		virtual ~Application();

		void Run();
		virtual void OnUpdate(f64 deltaTime) {}
		virtual void OnRender(f64 deltaTime) {}
	private:
		void Shutdown();

		bool m_running = false;
		bool m_suspended = false;
		bool m_initialized = false;

		f64 m_lastTime;

		SDL_Window* m_window{ nullptr };

		friend int ::main(int argc, char** argv);
	};

	Application* CreateApplication();
}