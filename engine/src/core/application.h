
#pragma once
#include "defines.h"
#include "logger.h"

#include "resources/geometry.h"
#include "resources/mesh.h"
#include "resources/skybox.h"

int main(int argc, char** argv);

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
		void Quit();

		virtual void OnCreate() {}
		virtual void OnUpdate(f64 deltaTime) {}
		virtual void OnRender(f64 deltaTime) {}

		virtual void OnResize(u16 width, u16 height) {}

		void GetFrameBufferSize(u32* width, u32* height) const;

		[[nodiscard]] SDL_Window* GetWindow() const;

		[[nodiscard]] const ApplicationState* GetState() const;
	private:
		void Shutdown();
		void HandleSdlEvents();

		bool OnResizeEvent(u16 code, void* sender, EventContext context);
		bool OnMinimizeEvent(u16 code, void* sender, EventContext context);
		bool OnFocusGainedEvent(u16 code, void* sender, EventContext context);

		// TEMP
		bool OnDebugEvent(u16 code, void* sender, EventContext context);
		// TEMP END

		static bool OnKeyEvent(u16 code, void* sender, EventContext context);

		LoggerInstance m_logger;
		ApplicationState m_state;

		SDL_Window* m_window{ nullptr };

		// TEMP
		Skybox m_skybox;

		Mesh m_meshes[10];
		u32 m_meshCount;

		Mesh m_uiMeshes[10];
		u32 m_uiMeshCount;
		// TEMP END

		friend int ::main(int argc, char** argv);
	};

	Application* CreateApplication();
	void DestroyApplication(const Application* app);
}