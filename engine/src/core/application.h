
#pragma once
#include "containers/string.h"
#include "renderer/renderer_plugin.h"
#include "systems/fonts/font_system.h"

namespace C3D
{
    class Engine;
    struct FrameData;

    struct ApplicationState
    {
        String name;

        i32 x, y;
        i32 width, height;

        RendererPlugin* rendererPlugin;
        FontSystemConfig fontConfig;

        DynamicArray<RenderView*> renderViews;

        /** @brief The size that should be allocated for the per-frame allocator. */
        u64 frameAllocatorSize;
        /** @brief The size required from the application-specific frame data. */
        u64 appFrameDataSize = 0;
    };

    class Application
    {
    public:
        explicit Application(ApplicationState* appState) : m_logger(appState->name.Data()), m_appState(appState) {}

        Application(const Application&) = delete;
        Application(Application&&)      = delete;

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&)      = delete;

        virtual ~Application() = default;

        virtual bool OnBoot()                    = 0;
        virtual bool OnRun(FrameData& frameData) = 0;

        virtual void OnUpdate(FrameData& frameData)                       = 0;
        virtual bool OnRender(RenderPacket& packet, FrameData& frameData) = 0;

        virtual void OnResize() = 0;

        virtual void OnShutdown() = 0;

        virtual void OnLibraryLoad()   = 0;
        virtual void OnLibraryUnload() = 0;

        friend Engine;

    protected:
        LoggerInstance<64> m_logger;

        ApplicationState* m_appState;
        UIConsole* m_pConsole                  = nullptr;
        const SystemManager* m_pSystemsManager = nullptr;
    };

    Application* CreateApplication();
    void InitApplication(Engine* engine);
    void DestroyApplication();
}  // namespace C3D
