
#include "engine.h"

#include "application.h"
#include "clock.h"
#include "containers/string.h"
#include "logger.h"
#include "metrics/metrics.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"
#include "systems/cameras/camera_system.h"
#include "systems/cvars/cvar_system.h"
#include "systems/events/event_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/input/input_system.h"
#include "systems/jobs/job_system.h"
#include "systems/lights/light_system.h"
#include "systems/materials/material_system.h"
#include "systems/render_views/render_view_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "ENGINE";

    Engine::Engine(Application* pApplication, UIConsole* pConsole)
        : m_application(pApplication), m_pConsole(pConsole), m_pSystemsManager(&m_systemsManager)
    {
        m_application->m_pSystemsManager = &m_systemsManager;
        m_application->m_pConsole        = pConsole;
    }

    void Engine::Init()
    {
        C3D_ASSERT_MSG(!m_state.initialized, "Tried to initialize the engine twice");

        const auto appState = m_application->m_appState;

        // Setup our frame allocator
        m_frameAllocator.Create("FRAME_ALLOCATOR", appState->frameAllocatorSize);
        m_frameData.frameAllocator = &m_frameAllocator;

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
            FATAL_LOG("System reported {} threads. C3DEngine requires at least 1 thread besides the main thread.", threadCount);
        }
        else
        {
            INFO_LOG("System reported {} threads (including main thread).", threadCount);
        }

        m_systemsManager.OnInit();

        String windowName = String::FromFormat("C3DEngine - {}", appState->name);

        constexpr ResourceSystemConfig resourceSystemConfig{ 32, "../../../assets" };
        constexpr ShaderSystemConfig shaderSystemConfig{ 128, 128, 31, 31 };
        const PlatformSystemConfig platformConfig{ windowName.Data(), appState->x, appState->y, appState->width, appState->height };
        const CVarSystemConfig cVarSystemConfig{ 31, m_pConsole };
        const RenderSystemConfig renderSystemConfig{ "TestEnv", appState->rendererPlugin, FlagVSyncEnabled | FlagPowerSavingEnabled };

        // Init before boot systems
        m_systemsManager.RegisterSystem<EventSystem>(EventSystemType);                              // Event System
        m_systemsManager.RegisterSystem<Platform>(PlatformSystemType, platformConfig);              // Platform (OS) System
        m_systemsManager.RegisterSystem<CVarSystem>(CVarSystemType, cVarSystemConfig);              // CVar System
        m_systemsManager.RegisterSystem<InputSystem>(InputSystemType);                              // Input System
        m_systemsManager.RegisterSystem<ResourceSystem>(ResourceSystemType, resourceSystemConfig);  // Resource System
        m_systemsManager.RegisterSystem<ShaderSystem>(ShaderSystemType, shaderSystemConfig);        // Shader System
        m_systemsManager.RegisterSystem<RenderSystem>(RenderSystemType, renderSystemConfig);        // Render System

        const auto rendererMultiThreaded = Renderer.IsMultiThreaded();

        m_application->OnBoot();

        constexpr auto maxThreadCount = 15;
        if (threadCount - 1 > maxThreadCount)
        {
            INFO_LOG("Available threads on this system is greater than {}. Capping used threads at {}.", maxThreadCount, (threadCount - 1),
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
        constexpr TextureSystemConfig textureSystemConfig{ 65536 };
        constexpr CameraSystemConfig cameraSystemConfig{ 61 };
        constexpr RenderViewSystemConfig viewSystemConfig{ 251 };

        m_systemsManager.RegisterSystem<JobSystem>(JobSystemType, jobSystemConfig);              // Job System
        m_systemsManager.RegisterSystem<TextureSystem>(TextureSystemType, textureSystemConfig);  // Texture System
        m_systemsManager.RegisterSystem<FontSystem>(FontSystemType, appState->fontConfig);       // Font System
        m_systemsManager.RegisterSystem<CameraSystem>(CameraSystemType, cameraSystemConfig);     // Camera System
        m_systemsManager.RegisterSystem<RenderViewSystem>(RenderViewSystemType,
                                                          viewSystemConfig);  // Render View System

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

        // Load render views
        for (auto view : appState->renderViews)
        {
            if (!Views.Register(view))
            {
                String name = view ? view->GetName() : "UNKOWN";
                FATAL_LOG("Failed to Create view: '{}'.", name);
            }
        }

        constexpr MaterialSystemConfig materialSystemConfig{ 4096 };
        constexpr GeometrySystemConfig geometrySystemConfig{ 4096 };

        m_systemsManager.RegisterSystem<MaterialSystem>(MaterialSystemType, materialSystemConfig);  // Material System
        m_systemsManager.RegisterSystem<GeometrySystem>(GeometrySystemType, geometrySystemConfig);  // Geometry System
        m_systemsManager.RegisterSystem<LightSystem>(LightSystemType);                              // Light System

        m_state.initialized = true;
        m_state.lastTime    = 0;

        m_application->OnResize();
        Renderer.OnResize(appState->width, appState->height);

        m_pConsole->OnInit(&m_systemsManager);
    }

    void Engine::Run()
    {
        m_state.running = true;

        Clock clock(m_systemsManager.GetSystemPtr<Platform>(PlatformSystemType));

        clock.Start();
        clock.Update();

        m_state.lastTime = clock.GetElapsed();

        u8 frameCount                    = 0;
        constexpr f64 targetFrameSeconds = 1.0 / 60.0;
        f64 frameElapsedTime             = 0;

        m_application->OnRun(m_frameData);

        Metrics.PrintMemoryUsage();

        while (m_state.running)
        {
            if (!OS.PumpMessages())
            {
                m_state.running = false;
            }

            if (!m_state.suspended)
            {
                clock.Update();
                const f64 currentTime    = clock.GetElapsed();
                const f64 delta          = currentTime - m_state.lastTime;
                const f64 frameStartTime = OS.GetAbsoluteTime();

                m_frameData.totalTime = currentTime;
                m_frameData.deltaTime = delta;

                // Reset our frame allocator (freeing all memory used previous frame)
                m_frameData.frameAllocator->FreeAll();

                Jobs.OnUpdate(m_frameData);
                Metrics.Update(frameElapsedTime);
                OS.WatchFiles();

                OnUpdate();

                // Reset our drawn mesh count for the next frame
                m_frameData.drawnMeshCount = 0;

                // This frame's render packet
                RenderPacket packet;
                // Ensure that we will be using our frame allocator for this packet's view
                packet.views.SetAllocator(&m_frameAllocator);

                // Have the application generate our render packet
                bool result = m_application->OnPrepareRenderPacket(packet, m_frameData);
                if (!result)
                {
                    ERROR_LOG("OnPrepareRenderPacket() failed. Skipping this frame.");
                    continue;
                }

                // Call the game's render routine
                if (!m_application->OnRender(packet, m_frameData))
                {
                    FATAL_LOG("OnRender() failed. Shutting down.");
                    m_state.running = false;
                    break;
                }

                // Cleanup our packets
                for (auto& view : packet.views)
                {
                    Views.DestroyPacket(view.view, view);
                }

                const f64 frameEndTime = OS.GetAbsoluteTime();
                frameElapsedTime       = frameEndTime - frameStartTime;

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

                Input.OnUpdate(m_frameData);

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

        const auto jan = 0xAFFFFFFF;
    }

    void Engine::OnResize(const u32 width, const u32 height) const
    {
        m_application->OnResize();
        Renderer.OnResize(width, height);
    }

    void Engine::GetFrameBufferSize(u32* width, u32* height) const
    {
        const auto appState = m_application->m_appState;

        *width  = appState->width;
        *height = appState->height;
    }

    const EngineState* Engine::GetState() const { return &m_state; }

    void Engine::OnApplicationLibraryReload(Application* app)
    {
        m_application                    = app;
        m_application->m_pSystemsManager = &m_systemsManager;
        m_application->m_pConsole        = m_pConsole;
        m_application->OnLibraryLoad();
    }

    void Engine::Shutdown()
    {
        C3D_ASSERT_MSG(m_state.initialized, "Tried to Shutdown application that hasn't been initialized")

        INFO_LOG("Shutting down.");

        // Call the OnShutdown() method that is defined by the user
        m_application->OnShutdown();

        // Destroy our frame allocator since we will no longer render any frames
        m_frameAllocator.Destroy();

        // Free the application specific frame data
        Memory.Free(MemoryType::Game, m_frameData.applicationFrameData);

        // Shutdown our console
        m_pConsole->OnShutDown();

        // Finally our systems manager can be shutdoen
        m_systemsManager.OnShutdown();

        m_state.initialized = false;
    }

    bool Engine::OnResizeEvent(const u16 code, void* sender, const EventContext& context)
    {
        if (code == EventCodeResized)
        {
            const u16 width     = context.data.u16[0];
            const u16 height    = context.data.u16[1];
            const auto appState = m_application->m_appState;

            // We only update out width and height if they actually changed
            if (width != appState->width || height != appState->height)
            {
                INFO_LOG("width: '{}' and height: '{}'.", width, height);

                const auto appState = m_application->m_appState;
                appState->width     = width;
                appState->height    = height;

                if (width == 0 || height == 0)
                {
                    INFO_LOG("Window minimized, suspending application.");
                    m_state.suspended = true;
                    return true;
                }

                m_state.suspended = false;
                OnResize(width, height);
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
