
#include "engine.h"

#include "application.h"
#include "console/console_sink.h"
#include "logger/logger.h"
#include "metrics/metrics.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"
#include "string/string.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/audio/audio_system.h"
#include "systems/cameras/camera_system.h"
#include "systems/cvars/cvar_system.h"
#include "systems/events/event_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/input/input_system.h"
#include "systems/jobs/job_system.h"
#include "systems/lights/light_system.h"
#include "systems/materials/material_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"
#include "systems/transforms/transform_system.h"
#include "time/clock.h"

namespace C3D
{
    Engine::Engine(Application* pApplication) : m_application(pApplication)
    {
        m_application->m_pConsole = &m_console;
        Logger::AddSink(std::make_shared<ConsoleSink>(&m_console));
    }

    bool Engine::Init()
    {
        C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the engine twice");

        INFO_LOG("Initializing.");

        // Get a quick reference to our app config
        const auto& appConfig = m_application->m_appConfig;

        auto threadCount = Platform::GetProcessorCount();
        if (threadCount <= 1)
        {
            ERROR_LOG("System reported: {} threads. C3DEngine requires at least 1 thread besides the main thread.", threadCount);
            return false;
        }

        INFO_LOG("System reported: {} threads (including main thread).", threadCount);

        // Setup our frame allocator
        if (appConfig.frameAllocatorSize < MebiBytes(8))
        {
            ERROR_LOG("Frame allocator must be >= 8 Mebibytes.");
            return false;
        }

        m_frameAllocator.Create("FRAME_ALLOCATOR", appConfig.frameAllocatorSize);
        m_frameData.allocator = &m_frameAllocator;

        SystemManager::OnInit();

        Platform::SetOnQuitCallback([this]() { Quit(); });
        Platform::SetOnResizeCallback([this](u16 width, u16 height) { OnResizeEvent(width, height); });

        // Init before boot systems
        SystemManager::RegisterSystem<EventSystem>(EventSystemType);                                                // Event System
        SystemManager::RegisterSystem<CVarSystem>(CVarSystemType, appConfig.systemConfigs["CVar"]);                 // CVar System
        SystemManager::RegisterSystem<InputSystem>(InputSystemType);                                                // Input System
        SystemManager::RegisterSystem<ResourceSystem>(ResourceSystemType, appConfig.systemConfigs["Resource"]);     // Resource System
        SystemManager::RegisterSystem<ShaderSystem>(ShaderSystemType, appConfig.systemConfigs["Shader"]);           // Shader System
        SystemManager::RegisterSystem<TransformSystem>(TransformSystemType, appConfig.systemConfigs["Transform"]);  // Transform System

        // After the Event system is up and running we register an OnQuit event
        Event.Register(EventCodeApplicationQuit, [this](u16 code, void* sender, const EventContext& context) {
            Quit();
            return true;
        });

        // Create all the requested windows
        for (const auto& windowConfig : appConfig.windowConfigs)
        {
            if (!Platform::CreateWindow(windowConfig))
            {
                ERROR_LOG("Failed to create a window.");
                return false;
            }
        }

        // We must initialize the Texture system first since our RenderSystem depends on it
        SystemManager::RegisterSystem<TextureSystem>(TextureSystemType, appConfig.systemConfigs["Texture"]);  // Texture System
        SystemManager::RegisterSystem<RenderSystem>(RenderSystemType, appConfig.systemConfigs["Renderer"]);   // Render System

        // But we can only create default textures once we have our RenderSystem running
        Textures.CreateDefaultTextures();

        SystemManager::RegisterSystem<UI2DSystem>(UI2DSystemType, appConfig.systemConfigs["UI2D"]);     // UI2D System
        SystemManager::RegisterSystem<AudioSystem>(AudioSystemType, appConfig.systemConfigs["Audio"]);  //  Audio System

        const auto rendererMultiThreaded = Renderer.IsMultiThreaded();

        // Ensure the application can access the engine before we start calling into application code
        m_application->m_pEngine = this;

        // Try to boot the application
        if (!m_application->OnBoot())
        {
            ERROR_LOG("Application failed to boot!");
            return false;
        }

        SystemManager::RegisterSystem<JobSystem>(JobSystemType, appConfig.systemConfigs["Job"]);           // Job System
        SystemManager::RegisterSystem<FontSystem>(FontSystemType, appConfig.systemConfigs["Font"]);        // Font System
        SystemManager::RegisterSystem<CameraSystem>(CameraSystemType, appConfig.systemConfigs["Camera"]);  // Camera System

        SystemManager::RegisterSystem<MaterialSystem>(MaterialSystemType, appConfig.systemConfigs["Material"]);  // Material System
        SystemManager::RegisterSystem<GeometrySystem>(GeometrySystemType, appConfig.systemConfigs["Geometry"]);  // Geometry System
        SystemManager::RegisterSystem<LightSystem>(LightSystemType);                                             // Light System

        m_state.initialized = true;
        m_state.lastTime    = 0;

        // Get the window size from the OS since it could be any size (depending on the options that the user requested)
        auto size            = Platform::GetWindowSize();
        m_state.windowWidth  = size.x;
        m_state.windowHeight = size.y;

        // Initialize our console
        m_console.OnInit();

        INFO_LOG("Successfully initialized.");
        return true;
    }

    void Engine::Run()
    {
        INFO_LOG("Started.");

        m_state.running  = true;
        m_state.lastTime = Platform::GetAbsoluteTime();

        UI2D.OnRun();

        m_console.OnRun();
        m_application->OnRun(m_frameData);
        OnResize(m_state.windowWidth, m_state.windowHeight);

        Metrics.PrintMemoryUsage();

        while (m_state.running)
        {
            if (!Platform::PumpMessages())
            {
                m_state.running = false;
            }

            if (!m_state.suspended)
            {
                m_state.clocks.total.Begin();

                const f64 currentTime = Platform::GetAbsoluteTime();
                const f64 delta       = currentTime - m_state.lastTime;

                m_frameData.timeData.total += delta;
                m_frameData.timeData.delta = delta;

                // Reset our frame allocator (freeing all memory used previous frame)
                m_frameData.allocator->FreeAll();

                Jobs.OnUpdate(m_frameData);
                Metrics.Update(m_frameData, m_state.clocks);
                Platform::WatchFiles();

                if (m_state.resizing)
                {
                    m_state.framesSinceResize++;

                    if (m_state.framesSinceResize >= 5)
                    {
                        OnResize(m_state.windowWidth, m_state.windowHeight);
                    }
                    else
                    {
                        // Simulate a 60FPS frame
                        Platform::SleepMs(16);
                    }

                    // No need to do other logic since we are still resizing
                    continue;
                }

                m_state.clocks.prepareFrame.Begin();

                if (!Renderer.PrepareFrame(m_frameData))
                {
                    // If we fail to prepare the frame we just skip this frame since we are propabably just done resizing
                    // or we just changed a renderer flag (like VSYNC) which will require resource recreation and will skip a frame.
                    // Notify our application of the resize
                    m_application->OnResize(m_state.windowWidth, m_state.windowHeight);
                    continue;
                }

                m_state.clocks.prepareFrame.End();

                m_state.clocks.onUpdate.Begin();

                OnUpdate();

                m_state.clocks.onUpdate.End();

                // Reset our drawn mesh count for the next frame
                m_frameData.drawnMeshCount = 0;

                if (!Renderer.Begin(m_frameData))
                {
                    FATAL_LOG("Renderer.Begin() failed. Shutting down.");
                    m_state.running = false;
                    break;
                }

                m_state.clocks.prepareRender.Begin();

                Renderer.BeginDebugLabel("PrepareRender", vec3(1.0f, 1.0f, 0.0f));

                SystemManager::OnPrepareRender(m_frameData);

                // Let the application prepare all the data for the next frame
                bool prepareFrameResult = m_application->OnPrepareRender(m_frameData);

                Renderer.EndDebugLabel();

                if (!prepareFrameResult)
                {
                    // We skip this frame since we failed to prepare our render
                    continue;
                }

                m_state.clocks.prepareRender.End();

                m_state.clocks.onRender.Begin();

                // Call the game's render routine
                if (!m_application->OnRender(m_frameData))
                {
                    FATAL_LOG("OnRender() failed. Shutting down.");
                    m_state.running = false;
                    break;
                }

                m_state.clocks.onRender.End();

                // End the frame
                Renderer.End(m_frameData);

                m_state.clocks.present.Begin();

                // Present our frame
                if (!Renderer.Present(m_frameData))
                {
                    ERROR_LOG("Failed to present the Renderer.");
                    m_state.running = false;
                    break;
                }

                m_state.clocks.present.End();

                Input.OnUpdate(m_frameData);

                m_state.clocks.total.End();
                m_state.lastTime = currentTime;
            }
        }

        Shutdown();

        INFO_LOG("Finished.");
    }

    void Engine::Quit() { m_state.running = false; }

    void Engine::OnUpdate()
    {
        m_console.OnUpdate();
        m_application->OnUpdate(m_frameData);
    }

    void Engine::OnResize(const u32 width, const u32 height)
    {
        // Notify our renderer of the resize
        Renderer.OnResize(width, height);
        // Prepare our next frame
        Renderer.PrepareFrame(m_frameData);
        // Notify our application of the resize
        m_application->OnResize(width, height);

        m_state.framesSinceResize = 0;
        m_state.resizing          = false;
    }

    void Engine::OnApplicationLibraryReload(Application* app)
    {
        m_application             = app;
        m_application->m_pConsole = &m_console;
        m_application->OnLibraryLoad();
    }

    void Engine::Shutdown()
    {
        INFO_LOG("Shutting down.");

        // Call the OnShutdown() method that is defined by the user
        if (m_application)
        {
            m_application->OnShutdown();
        }

        // Destroy our frame allocator since we will no longer render any frames
        m_frameAllocator.Destroy();

        // Shutdown our console
        m_console.OnShutDown();

        // Finally our systems manager can be shut down
        SystemManager::OnShutdown();

        m_state.initialized = false;
    }

    bool Engine::OnResizeEvent(u16 width, u16 height)
    {
        // Flag that we are currently resizing
        m_state.resizing = true;
        // Start counting the frames since the last resize
        m_state.framesSinceResize = 0;

        // We only update out width and height if they actually changed
        if (width != m_state.windowWidth || height != m_state.windowHeight)
        {
            INFO_LOG("width: '{}' and height: '{}'.", width, height);

            m_state.windowWidth  = width;
            m_state.windowHeight = height;

            if (width == 0 || height == 0)
            {
                INFO_LOG("Window minimized, suspending application.");
                m_state.suspended = true;
                return true;
            }

            m_state.suspended = false;
        }

        return false;
    }
}  // namespace C3D
