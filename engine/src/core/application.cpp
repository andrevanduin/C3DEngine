
#include "application.h"
#include "logger.h"
#include "input.h"
#include "clock.h"
#include "containers/string.h"

#include <SDL2/SDL.h>

#include "events/event.h"
#include "platform/platform.h"

#include "services/services.h"

#include "renderer/renderer_frontend.h"
#include "systems/geometry_system.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"
#include "systems/camera_system.h"
#include "systems/render_view_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	Application::Application(ApplicationConfig config)
		: m_logger("APPLICATION"), m_config(std::move(config))
	{}

	Application::~Application() = default;

	void Application::Init()
	{
		C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the application twice");

		m_state.name = m_config.name;
		m_state.width = m_config.width;
		m_state.height = m_config.height;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			m_logger.Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		m_logger.Info("Successfully initialized SDL");

		String windowName = "C3DEngine - ";
		windowName.Append(m_config.name);

		m_window = SDL_CreateWindow(windowName.Data(), m_config.x, m_config.y, m_config.width, m_config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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

		constexpr ResourceSystemConfig		resourceSystemConfig{ 32, "../../../../assets" };
		constexpr ShaderSystemConfig		shaderSystemConfig{ 128, 128, 31, 31 };

		Services::InitBeforeBoot(this, resourceSystemConfig, shaderSystemConfig);

		const auto rendererMultiThreaded = Renderer.IsMultiThreaded();

		OnBoot();

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

		Services::InitAfterBoot(jobSystemConfig, textureSystemConfig, m_config.fontConfig, cameraSystemConfig, viewSystemConfig);

		Event.Register(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.Register(SystemEventCode::Minimized, new EventCallback(this, &Application::OnMinimizeEvent));
		Event.Register(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));

		// Load render views
		for (auto& view : m_config.renderViews)
		{
			if (!Views.Create(view))
			{
				m_logger.Fatal("Failed to Create view: '{}'", view.name);
			}
		}

		constexpr MaterialSystemConfig materialSystemConfig{ 4096 };
		constexpr GeometrySystemConfig geometrySystemConfig{ 4096 };

		Services::FinalInit(materialSystemConfig, geometrySystemConfig);

		m_state.initialized = true;
		m_state.lastTime = 0;

		Application::OnResize(m_state.width, m_state.height);
		Renderer.OnResize(m_state.width, m_state.height);
	}

	void Application::Run()
	{
		m_state.running = true;

		Clock clock;

		clock.Start();
		clock.Update();

		m_state.lastTime = clock.GetElapsed();

		u8 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;
		f64 frameElapsedTime = 0;

		OnCreate();

		Metrics.PrintMemoryUsage();

		while (m_state.running)
		{
			HandleSdlEvents();

			if (!m_state.suspended)
			{
				clock.Update();
				const f64 currentTime = clock.GetElapsed();
				const f64 delta = currentTime - m_state.lastTime;
				const f64 frameStartTime = Platform::GetAbsoluteTime();

				Jobs.Update();
				Metrics.Update(frameElapsedTime);

				OnUpdate(delta);

				// TODO: Refactor packet creation
				RenderPacket packet;
				OnRender(packet, delta);

				if (!Renderer.DrawFrame(&packet))
				{
					m_logger.Warn("DrawFrame() failed");
				}

				// Cleanup our packets
				for (auto& view : packet.views)
				{
					Views.DestroyPacket(view.view, view);
				}

				const f64 frameEndTime = Platform::GetAbsoluteTime();
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

	void Application::Quit()
	{
		m_state.running = false;
	}

	void Application::GetFrameBufferSize(u32* width, u32* height) const
	{
		*width = m_state.width;
		*height = m_state.height;
	}

	SDL_Window* Application::GetWindow() const
	{
		return m_window;
	}

	const ApplicationState* Application::GetState() const
	{
		return &m_state;
	}

	void Application::Shutdown()
	{
		C3D_ASSERT_MSG(m_state.initialized, "Tried to Shutdown application that hasn't been initialized")

		// Call the OnShutdown() method that is defined by the user
		OnShutdown();

		m_logger.Info("Shutdown()");
		m_logger.Info("UnRegistering events");

		Event.UnRegister(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.UnRegister(SystemEventCode::Minimized, new EventCallback(this, &Application::OnMinimizeEvent));
		Event.UnRegister(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));

		Services::Shutdown();

		SDL_DestroyWindow(m_window);

		m_state.initialized = false;
	}

	void Application::HandleSdlEvents()
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
				case SDL_KEYUP:
					Services::GetInput().ProcessKey(e.key.keysym.sym, e.type == SDL_KEYDOWN);
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					Services::GetInput().ProcessButton(e.button.button, e.type == SDL_MOUSEBUTTONDOWN);
					break;
				case SDL_MOUSEMOTION:
					Services::GetInput().ProcessMouseMove(e.motion.x, e.motion.y);
					break;
				case SDL_MOUSEWHEEL:
					Services::GetInput().ProcessMouseWheel(e.wheel.y);
					break;
				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						EventContext context{};
						context.data.u16[0] = static_cast<u16>(e.window.data1);
						context.data.u16[1] = static_cast<u16>(e.window.data2);
						Event.Fire(SystemEventCode::Resized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
					{
						constexpr EventContext context{};
						Event.Fire(SystemEventCode::Minimized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_ENTER && m_state.suspended)
					{
						EventContext context{};
						context.data.u16[0] = static_cast<u16>(m_state.width);
						context.data.u16[1] = static_cast<u16>(m_state.height);
						Event.Fire(SystemEventCode::FocusGained, nullptr, context);
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

	bool Application::OnResizeEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::Resized)
		{
			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			// We only update out width and height if they actually changed
			if (width != m_state.width || height != m_state.height)
			{
				m_logger.Debug("Window Resize: {} {}", width, height);

				if (width == 0 || height == 0)
				{
					m_logger.Warn("Invalid width or height");
					return true;
				}

				m_state.width = width;
				m_state.height = height;

				OnResize(width, height);
				Renderer.OnResize(width, height);
			}
		}

		return false;
	}

	bool Application::OnMinimizeEvent(const u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::Minimized)
		{
			m_logger.Info("Window was minimized - suspending application");
			m_state.suspended = true;
		}

		return false;
	}

	bool Application::OnFocusGainedEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::FocusGained)
		{
			m_logger.Info("Window has regained focus - resuming application");
			m_state.suspended = false;

			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			OnResize(width, height);
			Renderer.OnResize(width, height);
		}

		return false;
	}
}
