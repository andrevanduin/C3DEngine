
#include "application.h"
#include "logger.h"
#include "memory.h"
#include "input.h"
#include "clock.h"

#include <SDL2/SDL.h>

#include "events/event.h"
#include "platform/platform.h"

#include "services/services.h"

// TEMP
#include "math/geometry_utils.h"
#include "core/c3d_string.h"
//

#include "renderer/renderer_frontend.h"
#include "systems/geometry_system.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

namespace C3D
{
	Application::Application(const ApplicationConfig& config)
		: m_logger("APPLICATION"), m_testGeometry(nullptr)
	{
		C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the application twice")

		Logger::Init();
		m_logger.Info("Starting");

		m_state.name = config.name;
		m_state.width = config.width;
		m_state.height = config.height;

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			m_logger.Fatal("Failed to initialize SDL: {}", SDL_GetError());
		}

		m_logger.Info("Successfully initialized SDL");

		m_window = SDL_CreateWindow(("C3DEngine - " + config.name).c_str(), config.x, config.y, config.width, config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
		{
			m_logger.Fatal("Failed to create a Window: {}", SDL_GetError());
		}

		m_logger.Info("Successfully created SDL Window");

		constexpr MemorySystemConfig	memorySystemConfig		{ MebiBytes(512) };
		constexpr ResourceSystemConfig	resourceSystemConfig	{ 32, "../../../../assets" };
		constexpr ShaderSystemConfig	shaderSystemConfig		{ 128, 128, 31, 31 };
		constexpr TextureSystemConfig	textureSystemConfig		{ 65536 };
		constexpr MaterialSystemConfig	materialSystemConfig	{ 4096 };
		constexpr GeometrySystemConfig	geometrySystemConfig	{ 4096 };

		Services::Init(this, memorySystemConfig, resourceSystemConfig, shaderSystemConfig, textureSystemConfig, materialSystemConfig, geometrySystemConfig);

		Event.Register(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.Register(SystemEventCode::Minimized,  new EventCallback(this, &Application::OnMinimizeEvent));
		Event.Register(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));
		Event.Register(SystemEventCode::Debug0, new EventCallback(this, &Application::OnDebugEvent));

		Event.Register(SystemEventCode::KeyPressed, new StaticEventCallback(&OnKeyEvent));
		Event.Register(SystemEventCode::KeyReleased, new StaticEventCallback(&OnKeyEvent));

		// TEMP
		// Load up a plane configuration, and load geometry for it
		const GeometryConfig gConfig = Geometric.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test geometry", "test_material");
		GeometryUtils::GenerateTangents(static_cast<Vertex3D*>(gConfig.vertices), gConfig.indexCount, static_cast<u32*>(gConfig.indices));

		m_testGeometry = Geometric.AcquireFromConfig(gConfig, true);

		// Cleanup the allocations that we just did
		Memory.Free(gConfig.vertices, sizeof(Vertex3D) * gConfig.vertexCount, MemoryType::Array);
		Memory.Free(gConfig.indices, sizeof(u32) * gConfig.indexCount, MemoryType::Array);

		// Load up our test ui geometry
		GeometryConfig uiConfig;
		uiConfig.vertexSize = sizeof(Vertex2D);
		uiConfig.vertexCount = 4;
		uiConfig.indexSize = sizeof(u32);
		uiConfig.indexCount = 6;
		StringNCopy(uiConfig.materialName, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
		StringNCopy(uiConfig.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

		constexpr f32 w = 128.0f;
		constexpr f32 h = 32.0f;

		Vertex2D uiVertices[4];
		uiVertices[0].position.x = 0.0f;
		uiVertices[0].position.y = 0.0f;
		uiVertices[0].texture.x = 0.0f;
		uiVertices[0].texture.y = 0.0f;

		uiVertices[1].position.x = w;
		uiVertices[1].position.y = h;
		uiVertices[1].texture.x = 1.0f;
		uiVertices[1].texture.y = 1.0f;

		uiVertices[2].position.x = 0.0f;
		uiVertices[2].position.y = h;
		uiVertices[2].texture.x = 0.0f;
		uiVertices[2].texture.y = 1.0f;

		uiVertices[3].position.x = w;
		uiVertices[3].position.y = 0.0f;
		uiVertices[3].texture.x = 1.0f;
		uiVertices[3].texture.y = 0.0f;

		uiConfig.vertices = uiVertices;

		// Counter-clockwise
		u32 uiIndices[6] = { 2, 1, 0, 3, 0, 1 };
		uiConfig.indices = uiIndices;

		m_testUiGeometry = Geometric.AcquireFromConfig(uiConfig, true);
		// TEMP END

		m_state.initialized = true;
		m_state.lastTime = 0;
	}

	Application::~Application() = default;

	void Application::Run()
	{
		m_state.running = true;

		Clock clock;

		clock.Start();
		clock.Update();

		m_state.lastTime = clock.GetElapsed();

		OnCreate();

		f64 runningTime = 0;
		u16 frameCount = 0;
		constexpr f64 targetFrameSeconds = 1.0 / 60.0;

		m_logger.Info(Memory.GetMemoryUsageString());

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

				// TEMP
				GeometryRenderData testRender{};
				testRender.geometry = m_testGeometry;
				//testRender.model = mat4(1.0f);
				static f32 angle = 0;
				angle += 0.5f * static_cast<f32>(delta);
				quat rotation = angleAxis(angle, vec3{ 0, 1, 0 });
				testRender.model = mat4(rotation);

				packet.geometryCount = 1;
				packet.geometries = &testRender;

				GeometryRenderData testUiRender{};
				testUiRender.geometry = m_testUiGeometry;
				testUiRender.model = mat4(1.0f);

				packet.uiGeometryCount = 1;
				packet.uiGeometries = &testUiRender;
				// TEMP END

				if (!Renderer.DrawFrame(&packet))
				{
					m_logger.Warn("DrawFrame() failed");
				}

				const f64 frameEndTime = Platform::GetAbsoluteTime();
				const f64 frameElapsedTime = frameEndTime - frameStartTime;
				runningTime += frameElapsedTime;
				const f64 remainingSeconds = targetFrameSeconds - frameElapsedTime;

				if (remainingSeconds > 0)
				{
					/*const u64 remainingMs = static_cast<u64>(remainingSeconds) * 1000;

					constexpr bool limitFrames = false;
					/if (remainingMs > 0 && limitFrames)
					{
						Platform::SleepMs(remainingMs - 1);
					}*/

					frameCount++;
				}

				Services::GetInput().Update(delta);

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

		m_logger.Info("Shutdown()");
		m_logger.Info("UnRegistering events");

		Event.UnRegister(SystemEventCode::Resized, new EventCallback(this, &Application::OnResizeEvent));
		Event.UnRegister(SystemEventCode::Minimized, new EventCallback(this, &Application::OnMinimizeEvent));
		Event.UnRegister(SystemEventCode::FocusGained, new EventCallback(this, &Application::OnFocusGainedEvent));
		// TEMP
		Event.UnRegister(SystemEventCode::Debug0, new EventCallback(this, &Application::OnDebugEvent));
		// TEMP END

		Event.UnRegister(SystemEventCode::KeyPressed, new StaticEventCallback(&OnKeyEvent));
		Event.UnRegister(SystemEventCode::KeyReleased, new StaticEventCallback(&OnKeyEvent));

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
						EventContext context;
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
				Services::GetRenderer().OnResize(width, height);
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
			Services::GetRenderer().OnResize(width, height);
		}

		return false;
	}

	// TEMP
	bool Application::OnDebugEvent(u16 code, void* sender, EventContext context)
	{
		const char* names[3] = { "cobblestone", "paving", "paving2" };
		const char* specNames[3] = { "cobblestone_specular", "paving_specular", "paving2_specular" };
		const char* normNames[3] = { "cobblestone_normal", "paving_normal", "paving2_normal" };

		static i8 choice = 2;

		const char* oldName = names[choice];
		const char* oldSpecularName = specNames[choice];
		const char* oldNormalName = normNames[choice];

		choice++;
		choice %= 3;

		if (m_testGeometry)
		{
			m_testGeometry->material->diffuseMap.texture = Textures.Acquire(names[choice], true);
			if (!m_testGeometry->material->diffuseMap.texture)
			{
				Logger::Warn("[ON_DEBUG_EVENT] - No diffuse texture! Using default");
				m_testGeometry->material->diffuseMap.texture = Textures.GetDefault();
			}

			// Release old diffuse texture
			Textures.Release(oldName);

			m_testGeometry->material->specularMap.texture = Textures.Acquire(specNames[choice], true);
			if (!m_testGeometry->material->specularMap.texture)
			{
				Logger::Warn("[ON_DEBUG_EVENT] - No specular texture! Using default");
				m_testGeometry->material->specularMap.texture = Textures.GetDefaultSpecular();
			}

			// Release old specular texture
			Textures.Release(oldSpecularName);

			m_testGeometry->material->normalMap.texture = Textures.Acquire(normNames[choice], true);
			if (!m_testGeometry->material->normalMap.texture)
			{
				Logger::Warn("[ON_DEBUG_EVENT] - No normal texture! Using default");
				m_testGeometry->material->normalMap.texture = Textures.GetDefaultNormal();
			}

			// Release old normal texture
			Textures.Release(oldNormalName);
		}

		return true;
	}
	// TEMP END

	bool Application::OnKeyEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::KeyPressed)
		{
			const auto key = context.data.u16[0];
			Logger::Debug("Key Pressed: {}", static_cast<char>(key));
		}
		else if (code == SystemEventCode::KeyReleased)
		{
			const auto key = context.data.u16[0];
			Logger::Debug("Key Released: {}", static_cast<char>(key));
		}
		return false;
	}
}
