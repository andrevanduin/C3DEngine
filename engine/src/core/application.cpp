
#include "application.h"
#include "logger.h"
#include "event.h"
#include "memory.h"
#include "input.h"
#include "clock.h"

#include <SDL2/SDL.h>

#include "platform/platform.h"
#include "renderer/renderer_frontend.h"

namespace C3D
{
	Application::Application(const ApplicationConfig& config)
	{
		C3D_ASSERT_MSG(!m_initialized, "Tried to initialize the application twice")

		Logger::PushPrefix("APPLICATION");
		Logger::Info("Starting");

		if (!EventSystem::Init())
		{
			Logger::Fatal("Event System failed to be Initialized");
		}

		if (!InputSystem::Init())
		{
			Logger::Fatal("Input System failed to be Initialized");
		}

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			Logger::Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		Logger::Info("Successfully initialized SDL");

		m_window = SDL_CreateWindow(("C3DEngine - " + config.name).c_str(), config.x, config.y, config.width, config.height, SDL_WINDOW_VULKAN);
		if (m_window == nullptr)
		{
			Logger::Fatal("Failed to create a Window: {}", SDL_GetError());
		}

		Logger::Info("Successfully created SDL Window");

		if (!Renderer::Init(config.name, m_window))
		{
			Logger::Fatal("Failed to initialize Renderer");
		}

		Logger::Info("Successfully created Renderer");
		Logger::PopPrefix();

		m_running = true;
		m_initialized = true;
		m_lastTime = 0;
	}

	Application::~Application() = default;

	void Application::Run()
	{
		Clock::Start();
		Clock::Update();

		m_lastTime = Clock::GetElapsed();

		f64 runningTime = 0;
		u8 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;

		Logger::Info(Memory::GetMemoryUsageString());
		
		SDL_Event e;
		while (m_running)
		{
			while (SDL_PollEvent(&e) != 0)
			{
				// TODO: ImGUI event process here
				if (e.type == SDL_QUIT) m_running = false;
				else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
				{
					InputSystem::ProcessKey(e.key.keysym.sym, e.type == SDL_KEYDOWN);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
				{
					InputSystem::ProcessButton(e.button.button, e.type == SDL_MOUSEBUTTONDOWN);
				}
				else if (e.type == SDL_MOUSEMOTION)
				{
					InputSystem::ProcessMouseMove(e.motion.x, e.motion.y);
				}
				else if (e.type == SDL_MOUSEWHEEL)
				{
					InputSystem::ProcessMouseWheel(e.wheel.y);
				}
			}

			if (!m_suspended)
			{
				Clock::Update();
				const f64 currentTime = Clock::GetElapsed();
				const f64 delta = currentTime - m_lastTime;
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

				m_lastTime = currentTime;
			}

			//SDL_Delay(100);
		}

		Shutdown();
	}

	void Application::Shutdown()
	{
		C3D_ASSERT_MSG(m_initialized, "Tried to Shutdown application that hasn't been initialized");

		Logger::PushPrefix("PLATFORM");
		Logger::Info("Shutdown()");

		InputSystem::Shutdown();
		EventSystem::Shutdown();
		Renderer::Shutdown();

		SDL_DestroyWindow(m_window);

		m_initialized = false;
		Logger::PopPrefix();
	}
}
