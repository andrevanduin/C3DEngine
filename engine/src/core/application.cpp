
#include "application.h"
#include "logger.h"
#include "memory.h"
#include "input.h"
#include "clock.h"

#include <SDL2/SDL.h>

#include "events/event.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"

namespace C3D
{
	Application::Application(const ApplicationConfig& config)
	{
		C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the application twice")

		Logger::PushPrefix("APPLICATION");
		Logger::Info("Starting");

		m_state.name = config.name;
		m_state.width = config.width;
		m_state.height = config.height;

		if (!EventSystem::Init())
		{
			Logger::Fatal("Event System failed to be Initialized");
		}

		EventSystem::Register(SystemEventCode::Resized, nullptr, new EventCallback(this, &Application::OnResizeEvent));
		EventSystem::Register(SystemEventCode::Minimized, nullptr, new EventCallback(this, &Application::OnMinimizeEvent));
		EventSystem::Register(SystemEventCode::FocusGained, nullptr, new EventCallback(this, &Application::OnFocusGainedEvent));
		EventSystem::Register(SystemEventCode::KeyPressed, nullptr, new EventCallback(this, &Application::OnKeyEvent));
		EventSystem::Register(SystemEventCode::KeyReleased, nullptr, new EventCallback(this, &Application::OnKeyEvent));

		if (!InputSystem::Init())
		{
			Logger::Fatal("Input System failed to be Initialized");
		}

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			Logger::Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		Logger::Info("Successfully initialized SDL");

		m_window = SDL_CreateWindow(("C3DEngine - " + config.name).c_str(), config.x, config.y, config.width, config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
		{
			Logger::Fatal("Failed to create a Window: {}", SDL_GetError());
		}

		Logger::Info("Successfully created SDL Window");

		if (!Renderer::Init(this))
		{
			Logger::Fatal("Failed to initialize Renderer");
		}

		Logger::Info("Successfully created Renderer");
		Logger::PopPrefix();

		m_state.running = true;
		m_state.initialized = true;
		m_state.lastTime = 0;
	}

	Application::~Application() = default;

	void Application::Run()
	{
		Clock clock;

		clock.Start();
		clock.Update();

		m_state.lastTime = clock.GetElapsed();

		f64 runningTime = 0;
		u8 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;

		Logger::Info(Memory::GetMemoryUsageString());

		while (m_state.running)
		{
			HandleSdlEvents();

			if (!m_state.suspended)
			{
				clock.Update();
				const f64 currentTime = clock.GetElapsed();
				const f64 delta = currentTime - m_state.lastTime;
				const f64 frameStartTime = Platform::GetAbsoluteTime();

				OnUpdate(delta);
				OnRender(delta);

				//TODO: Refactor this
				RenderPacket packet = { static_cast<f32>(delta) };
				Renderer::DrawFrame(&packet);

				const f64 frameEndTime = Platform::GetAbsoluteTime();
				const f64 frameElapsedTime = frameEndTime - frameStartTime;
				runningTime += frameElapsedTime;
				const f64 remainingSeconds = targetFrameSeconds - frameElapsedTime;

				if (remainingSeconds > 0)
				{
					const u64 remainingMs = static_cast<u64>(remainingSeconds) * 1000;

					constexpr bool limitFrames = false;
					if (remainingMs > 0 && limitFrames)
					{
						Platform::SleepMs(remainingMs - 1);
					}

					frameCount++;
				}

				InputSystem::Update(delta);

				m_state.lastTime = currentTime;
			}
		}

		Shutdown();
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

	void Application::Shutdown()
	{
		C3D_ASSERT_MSG(m_state.initialized, "Tried to Shutdown application that hasn't been initialized");

		Logger::PushPrefix("PLATFORM");
		Logger::Info("Shutdown()");

		InputSystem::Shutdown();
		EventSystem::Shutdown();
		Renderer::Shutdown();

		SDL_DestroyWindow(m_window);

		m_state.initialized = false;
		Logger::PopPrefix();
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
					InputSystem::ProcessKey(e.key.keysym.sym, e.type == SDL_KEYDOWN);
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					InputSystem::ProcessButton(e.button.button, e.type == SDL_MOUSEBUTTONDOWN);
					break;
				case SDL_MOUSEMOTION:
					InputSystem::ProcessMouseMove(e.motion.x, e.motion.y);
					break;
				case SDL_MOUSEWHEEL:
					InputSystem::ProcessMouseWheel(e.wheel.y);
					break;
				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						EventContext context;
						context.data.u16[0] = static_cast<u16>(e.window.data1);
						context.data.u16[1] = static_cast<u16>(e.window.data2);
						EventSystem::Fire(SystemEventCode::Resized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
					{
						constexpr EventContext context{};
						EventSystem::Fire(SystemEventCode::Minimized, nullptr, context);
					}
					else if (e.window.event == SDL_WINDOWEVENT_ENTER && m_state.suspended)
					{
						EventContext context;
						context.data.u16[0] = static_cast<u16>(m_state.width);
						context.data.u16[1] = static_cast<u16>(m_state.height);
						EventSystem::Fire(SystemEventCode::FocusGained, nullptr, context);
					}
					break;
				case SDL_TEXTINPUT:
					// TODO: Possibly change this is the future. But currently this spams the console if letters are pressed
					break;
				default:
					Logger::PrefixTrace("INPUT", "Unhandled SDL Event: {}", e.type);
					break;
			}
		}
	}

	bool Application::OnResizeEvent(const u16 code, void* sender, void* listener, const EventContext context)
	{
		if (code == SystemEventCode::Resized)
		{
			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			// We only update out width and height if they actually changed
			if (width != m_state.width || height != m_state.height)
			{
				Logger::Debug("Window Resize: {} {}", width, height);

				if (width == 0 || height == 0)
				{
					Logger::Warn("Invalid width or height");
					return true;
				}

				m_state.width = width;
				m_state.height = height;

				OnResize(width, height);
				Renderer::OnResize(width, height);
			}
		}

		return false;
	}

	bool Application::OnMinimizeEvent(const u16 code, void* sender, void* listener, EventContext context)
	{
		if (code == SystemEventCode::Minimized)
		{
			Logger::Info("Window was minimized - suspending application");
			m_state.suspended = true;
		}

		return false;
	}

	bool Application::OnFocusGainedEvent(const u16 code, void* sender, void* listener, const EventContext context)
	{
		if (code == SystemEventCode::FocusGained)
		{
			Logger::Info("Window has regained focus - resuming application");
			m_state.suspended = false;

			const u16 width = context.data.u16[0];
			const u16 height = context.data.u16[1];

			OnResize(width, height);
			Renderer::OnResize(width, height);
		}

		return false;
	}

	bool Application::OnKeyEvent(u16 code, void* sender, void* listener, EventContext context)
	{
		if (code == SystemEventCode::KeyPressed)
		{
			Logger::Debug("Key Pressed: {}", static_cast<char>(context.data.u16[0]));
		}
		else if (code == SystemEventCode::KeyReleased)
		{
			Logger::Debug("Key Released: {}", static_cast<char>(context.data.u16[0]));
		}
		return false;
	}
}
