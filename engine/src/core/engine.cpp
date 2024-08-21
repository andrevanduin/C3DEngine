
#include "engine.h"

#include "application.h"
#include "clock.h"
#include "containers/string.h"
#include "logger.h"
#include "metrics/metrics.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"
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

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "ENGINE";

    Engine::Engine(Application* pApplication, UIConsole* pConsole) : m_application(pApplication), m_pConsole(pConsole)
    {
        m_application->m_pConsole = pConsole;
    }

    bool Engine::Init()
    {
        C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the engine twice");

        const auto appState = m_application->m_appState;

        // Setup our frame allocator
        m_frameAllocator.Create("FRAME_ALLOCATOR", appState->frameAllocatorSize);
        m_frameData.allocator = &m_frameAllocator;

        // Allocate enough space for the application specific frame-data. Which is used only by the user from the
        // application itself
        if (appState->appFrameDataSize > 0)
        {
            m_frameData.applicationFrameData =
                static_cast<ApplicationFrameData*>(Memory.AllocateBlock(MemoryType::Game, appState->appFrameDataSize));
        }
        else
        {
            m_frameData.applicationFrameData = nullptr;
        }

        auto threadCount = Platform::GetProcessorCount();
        if (threadCount <= 1)
        {
            FATAL_LOG("System reported: {} threads. C3DEngine requires at least 1 thread besides the main thread.", threadCount);
        }
        else
        {
            INFO_LOG("System reported: {} threads (including main thread).", threadCount);
        }

        auto& systemsManager = SystemManager::GetInstance();
        systemsManager.OnInit();

        String windowName = String::FromFormat("C3DEngine - {}", appState->name);

        constexpr ResourceSystemConfig resourceSystemConfig{ 32, "../../../assets" };
        constexpr ShaderSystemConfig shaderSystemConfig;
        constexpr TextureSystemConfig textureSystemConfig{ 65536 };
        const PlatformSystemConfig platformConfig{ windowName.Data(), appState->windowConfig };
        const CVarSystemConfig cVarSystemConfig{ 31, m_pConsole };
        const RenderSystemConfig renderSystemConfig{ "TestEnv", appState->rendererPlugin,
                                                     FlagVSyncEnabled | FlagPowerSavingEnabled | FlagUseValidationLayers };
        constexpr UI2DSystemConfig ui2dSystemConfig{ 1024, MebiBytes(16) };
        constexpr AudioSystemConfig audioSystemConfig{ "C3DOpenAL", 0, ChannelType::Stereo, 4096 * 16, 8 };

        // Init before boot systems
        systemsManager.RegisterSystem<EventSystem>(EventSystemType);                              // Event System
        systemsManager.RegisterSystem<Platform>(PlatformSystemType, platformConfig);              // Platform (OS) System
        systemsManager.RegisterSystem<CVarSystem>(CVarSystemType, cVarSystemConfig);              // CVar System
        systemsManager.RegisterSystem<InputSystem>(InputSystemType);                              // Input System
        systemsManager.RegisterSystem<ResourceSystem>(ResourceSystemType, resourceSystemConfig);  // Resource System
        systemsManager.RegisterSystem<ShaderSystem>(ShaderSystemType, shaderSystemConfig);        // Shader System

        // We must initialize the Texture system first since our RenderSystem depends on it
        systemsManager.RegisterSystem<TextureSystem>(TextureSystemType, textureSystemConfig);  // Texture System
        systemsManager.RegisterSystem<RenderSystem>(RenderSystemType, renderSystemConfig);     // Render System

        // But we can only create default textures once we have our RenderSystem running
        Textures.CreateDefaultTextures();

        systemsManager.RegisterSystem<UI2DSystem>(UI2DSystemType, ui2dSystemConfig);     // UI2D System
        systemsManager.RegisterSystem<AudioSystem>(AudioSystemType, audioSystemConfig);  //  Audio System

        const auto rendererMultiThreaded = Renderer.IsMultiThreaded();

        // Ensure the application can access the engine before we start calling into application code
        m_application->m_pEngine = this;

        if (!m_application->OnBoot())
        {
            ERROR_LOG("Application failed to boot!");
            return false;
        }

        constexpr auto maxThreadCount = 15;
        if (threadCount - 1 > maxThreadCount)
        {
            INFO_LOG("Available threads on this system is > {}. Capping used threads at {}.", maxThreadCount, (threadCount - 1),
                     maxThreadCount);
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

        const JobSystemConfig jobSystemConfig{ static_cast<u8>(threadCount - 1), jobThreadTypes };
        constexpr CameraSystemConfig cameraSystemConfig{ 61 };

        systemsManager.RegisterSystem<JobSystem>(JobSystemType, jobSystemConfig);           // Job System
        systemsManager.RegisterSystem<FontSystem>(FontSystemType, appState->fontConfig);    // Font System
        systemsManager.RegisterSystem<CameraSystem>(CameraSystemType, cameraSystemConfig);  // Camera System

        Event.Register(EventCodeResized,
                       [this](const u16 code, void* sender, const EventContext& context) { return OnResizeEvent(code, sender, context); });
        Event.Register(EventCodeMinimized, [this](const u16 code, void* sender, const EventContext& context) {
            return OnMinimizeEvent(code, sender, context);
        });
        Event.Register(EventCodeFocusGained, [this](const u16 code, void* sender, const EventContext& context) {
            return OnFocusGainedEvent(code, sender, context);
        });
        Event.Register(EventCodeApplicationQuit,
                       [this](const u16 code, void* sender, const EventContext& context) { return OnQuitEvent(code, sender, context); });

        constexpr MaterialSystemConfig materialSystemConfig{ 4077 };
        constexpr GeometrySystemConfig geometrySystemConfig{ 4096 };

        systemsManager.RegisterSystem<MaterialSystem>(MaterialSystemType, materialSystemConfig);  // Material System
        systemsManager.RegisterSystem<GeometrySystem>(GeometrySystemType, geometrySystemConfig);  // Geometry System
        systemsManager.RegisterSystem<LightSystem>(LightSystemType);                              // Light System

        m_state.initialized = true;
        m_state.lastTime    = 0;

        // Get the window size from the OS since it could be any size (depending on the options that the user requested)
        auto size            = OS.GetWindowSize();
        m_state.windowWidth  = size.x;
        m_state.windowHeight = size.y;

        m_pConsole->OnInit();
        return true;
    }

    void Engine::Run()
    {
        m_state.running  = true;
        m_state.lastTime = OS.GetAbsoluteTime();

        UI2D.OnRun();

        m_pConsole->OnRun();
        m_application->OnRun(m_frameData);
        OnResize(m_state.windowWidth, m_state.windowHeight);

        Metrics.PrintMemoryUsage();

        while (m_state.running)
        {
            if (!OS.PumpMessages())
            {
                m_state.running = false;
            }

            if (!m_state.suspended)
            {
                m_state.clocks.total.Begin();

                const f64 currentTime = OS.GetAbsoluteTime();
                const f64 delta       = currentTime - m_state.lastTime;

                m_frameData.timeData.total += delta;
                m_frameData.timeData.delta = delta;

                // Reset our frame allocator (freeing all memory used previous frame)
                m_frameData.allocator->FreeAll();

                Jobs.OnUpdate(m_frameData);
                Metrics.Update(m_frameData, m_state.clocks);
                OS.WatchFiles();

                if (m_state.resizing)
                {
                    m_state.framesSinceResize++;

                    if (m_state.framesSinceResize >= 5)
                    {
                        const auto appState = m_application->m_appState;
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

                SystemManager::GetInstance().OnPrepareRender(m_frameData);

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
    }

    void Engine::Quit() { m_state.running = false; }

    void Engine::OnUpdate()
    {
        m_pConsole->OnUpdate();
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
        m_application->m_pConsole = m_pConsole;
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

        // Free the application specific frame data
        Memory.Delete(m_frameData.applicationFrameData);

        // Shutdown our console
        m_pConsole->OnShutDown();

        // Finally our systems manager can be shut down
        auto& systemsManager = SystemManager::GetInstance();
        systemsManager.OnShutdown();

        m_state.initialized = false;
    }

    bool Engine::OnResizeEvent(const u16 code, void* sender, const EventContext& context)
    {
        if (code == EventCodeResized)
        {
            // Flag that we are currently resizing
            m_state.resizing = true;
            // Start counting the frames since the last resize
            m_state.framesSinceResize = 0;

            const u16 width  = context.data.u16[0];
            const u16 height = context.data.u16[1];

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
        }

        return false;
    }

    bool Engine::OnQuitEvent(u16, void*, const EventContext&)
    {
        Quit();
        return true;
    }

    bool Engine::OnMinimizeEvent(const u16 code, void* sender, const EventContext& context)
    {
        if (code == EventCodeMinimized)
        {
            INFO_LOG("Window was minimized - suspending application.");
            m_state.suspended = true;
        }

        return false;
    }

    bool Engine::OnFocusGainedEvent(const u16 code, void*, const EventContext& context)
    {
        if (code == EventCodeFocusGained)
        {
            INFO_LOG("Window has regained focus - resuming application.");
            m_state.suspended = false;

            const u16 width  = context.data.u16[0];
            const u16 height = context.data.u16[1];

            OnResize(width, height);
        }

        return false;
    }
}  // namespace C3D
