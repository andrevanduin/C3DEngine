
#include "engine.h"
#include "logger.h"
#include "clock.h"

#include <SDL2/SDL.h>

#include "application.h"
#include "platform/platform.h"
#include "containers/string.h"

#include "metrics/metrics.h"

#include "renderer/renderer_frontend.h"

#include "systems/system_manager.h"
#include "systems/events/event_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/materials/material_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/textures/texture_system.h"
#include "systems/cameras/camera_system.h"
#include "systems/cvars/cvar_system.h"
#include "systems/input/input_system.h"
#include "systems/render_views/render_view_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	Engine::Engine(Application* application)
		: m_logger("ENGINE"), m_engine(this), m_console(nullptr), m_application(application)
	{
		m_application->m_engine = this;
	}

	void Engine::Init()
	{
		C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the engine twice");

		const auto appState = m_application->m_appState;
		m_state.name = appState->name;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			m_logger.Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		m_logger.Info("Successfully initialized SDL");

		String windowName = String::FromFormat("C3DEngine - {}", appState->name);
		m_window = SDL_CreateWindow(windowName.Data(), appState->x, appState->y, appState->width, appState->height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
		{
			m_logger.Fatal("Failed to create a Window: {}", SDL_GetError());
		}

		m_logger.Info("Successfully created SDL Window");

		auto threadCount = Platform::GetProcessorCount();
		if (threadCount <= 1)
		{
			m_logger.Fatal("System reported {} threads. C3DEngine requires at least 1 thread besides the main thread.", threadCount);
		}
		else
		{
			m_logger.Info("System reported {} threads (including main thread).", threadCount);
		}

		m_systemsManager.Init(this);

		constexpr	ResourceSystemConfig	resourceSystemConfig	{ 32, "../../../assets"	};
		constexpr	ShaderSystemConfig		shaderSystemConfig		{ 128, 128, 31, 31			};
		constexpr	CVarSystemConfig		cVarSystemConfig		{ 31						};
		const		RenderSystemConfig		renderSystemConfig		{ "TestEnv", appState->rendererPlugin, FlagVSyncEnabled | FlagPowerSavingEnabled};

		// Init before boot systems
		m_systemsManager.RegisterSystem<Platform>(PlatformSystemType);								// Platform (OS) System
		m_systemsManager.RegisterSystem<EventSystem>(EventSystemType);								// Event System
		m_systemsManager.RegisterSystem<CVarSystem>(CVarSystemType, cVarSystemConfig);				// CVar System
		m_systemsManager.RegisterSystem<InputSystem>(InputSystemType);								// Input System
		m_systemsManager.RegisterSystem<ResourceSystem>(ResourceSystemType, resourceSystemConfig);	// Resource System
		m_systemsManager.RegisterSystem<ShaderSystem>(ShaderSystemType, shaderSystemConfig);		// Shader System
		m_systemsManager.RegisterSystem<RenderSystem>(RenderSystemType, renderSystemConfig);		// Render System

		const auto rendererMultiThreaded = Renderer.IsMultiThreaded();

		m_application->OnBoot();

		constexpr auto maxThreadCount = 15;
		if (threadCount - 1 > maxThreadCount)
		{
			m_logger.Info("Available threads on this system is greater than {} (). Capping used threads at {}", maxThreadCount, (threadCount - 1), maxThreadCount);
			threadCount = maxThreadCount;
		}

		u32 jobThreadTypes[15];
		for (u32& jobThreadType : jobThreadTypes) jobThreadType = JobTypeGeneral;

		if (threadCount == 1 || !rendererMultiThreaded)
		{
			jobThreadTypes[0] |= (JobTypeGpuResource | JobTypeResourceLoad);
		}
		else if (threadCount == 2)
		{
			jobThreadTypes[0] |= JobTypeGpuResource;
			jobThreadTypes[1] |= JobTypeResourceLoad;
		}
		else
		{
			jobThreadTypes[0] = JobTypeGpuResource;
			jobThreadTypes[1] = JobTypeResourceLoad;
		}

		const JobSystemConfig				jobSystemConfig { maxThreadCount, jobThreadTypes };
		constexpr TextureSystemConfig		textureSystemConfig { 65536 };
		constexpr CameraSystemConfig		cameraSystemConfig { 61 };
		constexpr RenderViewSystemConfig	viewSystemConfig { 251 };

		m_systemsManager.RegisterSystem<JobSystem>(JobSystemType, jobSystemConfig);					// Job System
		m_systemsManager.RegisterSystem<TextureSystem>(TextureSystemType, textureSystemConfig);		// Texture System
		m_systemsManager.RegisterSystem<FontSystem>(FontSystemType, appState->fontConfig);		// Font System
		m_systemsManager.RegisterSystem<CameraSystem>(CameraSystemType, cameraSystemConfig);		// Camera System
		m_systemsManager.RegisterSystem<RenderViewSystem>(RenderViewSystemType, viewSystemConfig);  // Render View System

		Event.Register(EventCodeResized,	 [this](const u16 code, void* sender, const EventContext& context) { return OnResizeEvent(code, sender, context);		});
		Event.Register(EventCodeMinimized,	 [this](const u16 code, void* sender, const EventContext& context) { return OnMinimizeEvent(code, sender, context);		});
		Event.Register(EventCodeFocusGained, [this](const u16 code, void* sender, const EventContext& context) { return OnFocusGainedEvent(code, sender, context);	});

		// Load render views
		for (auto& view : appState->renderViews)
		{
			if (!Views.Create(view))
			{
				m_logger.Fatal("Failed to Create view: '{}'", view.name);
			}
		}

		constexpr MaterialSystemConfig materialSystemConfig{ 4096 };
		constexpr GeometrySystemConfig geometrySystemConfig{ 4096 };

		m_systemsManager.RegisterSystem<MaterialSystem>(MaterialSystemType, materialSystemConfig); // Material System
		m_systemsManager.RegisterSystem<GeometrySystem>(GeometrySystemType, geometrySystemConfig); // Geometry System

		m_state.initialized = true;
		m_state.lastTime = 0;

		m_application->OnResize();
		Renderer.OnResize(appState->width, appState->height);

		Console.OnInit(this);
	}

	void Engine::Run()
	{
		m_state.running = true;

		Clock clock(m_systemsManager.GetSystemPtr<Platform>(PlatformSystemType));

		clock.Start();
		clock.Update();

		m_state.lastTime = clock.GetElapsed();

		u8 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;
		f64 frameElapsedTime = 0;

		m_application->OnRun();

		Metrics.PrintMemoryUsage();

		while (m_state.running)
		{
			HandleSdlEvents();

			if (!m_state.suspended)
			{
				clock.Update();
				const f64 currentTime = clock.GetElapsed();
				const f64 delta = currentTime - m_state.lastTime;
				const f64 frameStartTime = OS.GetAbsoluteTime();

				Jobs.Update(delta);
				Metrics.Update(frameElapsedTime);
				OS.WatchFiles();

				OnUpdate(delta);

				// TODO: Refactor packet creation
				RenderPacket packet;
				OnRender(packet, delta);

				if (!Renderer.DrawFrame(&packet))
				{
					m_logger.Warn("DrawFrame() failed");
				}

				// TODO: Do we need an AfterRender() method?

				// Cleanup our packets
				for (auto& view : packet.views)
				{
					Views.DestroyPacket(view.view, view);
				}

				const f64 frameEndTime = OS.GetAbsoluteTime();
				frameElapsedTime = frameEndTime - frameStartTime;

				if (const f64 remainingSeconds = targetFrameSeconds - frameElapsedTime; remainingSeconds > 0)
				{
					const u64 remainingMs = static_cast<u64>(remainingSeconds) * 1000;

					constexpr bool limitFrames = true;
					if (remainingMs > 0 && limitFrames)
					{
						Platform::SleepMs(remainingMs - 1);
					}

					frameCount++;
				}

				Input.Update(delta);

				m_state.lastTime = currentTime;
			}
		}

		Shutdown();
	}

	void Engine::Quit()
	{
		m_state.running = false;
	}

	bool Engine::OnBoot() const
	{
		return m_application->OnBoot();
	}

	void Engine::OnUpdate(const f64 deltaTime) const
	{
		m_console->OnUpdate();
		m_application->OnUpdate(deltaTime);
	}

	bool Engine::OnRender(RenderPacket& packet, const f64 deltaTime) const
	{
		return m_application->OnRender(packet, deltaTime);
	}

	void Engine::OnResize(const u32 width, const u32 height) const
	{
		const auto appState = m_application->m_appState;
		appState->width  = width;
		appState->height = height;

		m_application->OnResize();
		Renderer.OnResize(width, height);
	}

	void Engine::GetFrameBufferSize(u32* width, u32* height) const
	{
		const auto appState = m_application->m_appState;

		*width  = appState->width;
		*height = appState->height;
	}

	SDL_Window* Engine::GetWindow() const
	{
		return m_window;
	}

	const EngineState* Engine::GetState() const
	{
		return &m_state;
	}

	void Engine::OnApplicationLibraryReload(Application* app)
	{
		m_application = app;
		m_application->m_engine = this;
	}

	void Engine::Shutdown()
	{
		C3D_ASSERT_MSG(m_state.initialized, "Tried to Shutdown application that hasn't been initialized")

		// Call the OnShutdown() method that is defined by the user
		m_application->OnShutdown();

		m_logger.Info("Shutdown()");
		m_logger.Info("UnRegistering events");

		Console.OnShutDown();

		m_systemsManager.Shutdown();
		
		SDL_DestroyWindow(m_window);

		m_state.initialized = false;
	}

	void Engine::HandleSdlEvents()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			// TODO: ImGUI event process here

			switch (e.type)
			{
				case SDL_QUIT:
					m_state.running = false;
					break;
				case SDL_KEYDOWN:
					Input.ProcessKey(e.key.keysym.sym, InputState::Down);
					break;
				case SDL_KEYUP:
					Input.ProcessKey(e.key.keysym.sym, InputState::Up);
					break;
				case SDL_MOUSEBUTTONDOWN:
					Input.ProcessButton(e.button.button, InputState::Down);
					break;
				case SDL_MOUSEBUTTONUP:
					Input.ProcessButton(e.button.button, InputState::Up);
					break;
				case SDL_MOUSEMOTION:
					Input.ProcessMouseMove(e.motion.x, e.motion.y);
					break;
				case SDL_MOUSEWHEEL:
					Input.ProcessMouseWheel(e.wheel.y);
					break;
				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						EventContext context{};
						context.data.u16[0] = static_cast<u16>(e.window.data1);
						context.data.u16[1] = static_cast<u16>(e.window.data2);
						Event.Fire(EventCodeResized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
					{
						constexpr EventContext context{};
						Event.Fire(EventCodeMinimized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_ENTER && m_state.suspended)
					{
						const auto appState = m_application->m_appState;

						EventContext context{};
						context.data.u16[0] = static_cast<u16>(appState->width);
						context.data.u16[1] = static_cast<u16>(appState->height);
						Event.Fire(EventCodeFocusGained, nullptr, context);
					}
					break;
				case SDL_TEXTINPUT:
					// TODO: Possibly change this is the future. But currently this spams the console if letters are pressed
					break;
				default:
					m_logger.Trace("Unhandled SDL Event: {}", e.type);
					break;
			}
		}
	}

	bool Engine::OnResizeEvent(const u16 code, void* sender, const EventContext& context) const
	{
		if (code == EventCodeResized)
		{
			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];
			const auto appState = m_application->m_appState;

			// We only update out width and height if they actually changed
			if (width != appState->width || height != appState->height)
			{
				m_logger.Debug("Window Resize: {} {}", width, height);

				if (width == 0 || height == 0)
				{
					m_logger.Warn("Invalid width or height");
					return true;
				}

				OnResize(width, height);
			}
		}

		return false;
	}

	bool Engine::OnMinimizeEvent(const u16 code, void* sender, const EventContext& context)
	{
		if (code == EventCodeMinimized)
		{
			m_logger.Info("Window was minimized - suspending application");
			m_state.suspended = true;
		}

		return false;
	}

	bool Engine::OnFocusGainedEvent(const u16 code, void*, const EventContext& context)
	{
		if (code == EventCodeFocusGained)
		{
			m_logger.Info("Window has regained focus - resuming application");
			m_state.suspended = false;

			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			OnResize(width, height);
		}

		return false;
	}
}
