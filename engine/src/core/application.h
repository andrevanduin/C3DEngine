
#pragma once
#include "containers/string.h"
#include "platform/platform_base.h"
#include "renderer/renderer_plugin.h"
#include "systems/fonts/font_system.h"

namespace C3D
{
    class Engine;
    struct FrameData;

    enum ApplicationFlag : u8
    {
        /** @brief No flags set. */
        ApplicationFlagNone = 0x0,
    };

    using ApplicationFlagBits = u8;

    struct ApplicationState
    {
        /** @brief The name of the application. */
        String name;
        /** @brief The configuration for the window. */
        WindowConfig windowConfig;
        /** @brief Flags that indicate certain properties about this application. */
        ApplicationFlagBits flags = ApplicationFlagNone;
        /** @brief The name of the Rendering plugin that you want to use. */
        const char* rendererPlugin;
        FontSystemConfig fontConfig;

        /** @brief The size that should be allocated for the per-frame allocator. */
        u64 frameAllocatorSize = 0;
        /** @brief The size required from the application-specific frame data. */
        u64 appFrameDataSize = 0;
    };

    class Application
    {
    public:
        explicit Application(ApplicationState* appState) : m_appState(appState) {}

        Application(const Application&) = delete;
        Application(Application&&)      = delete;

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&)      = delete;

        virtual ~Application() = default;

        virtual bool OnBoot()                    = 0;
        virtual bool OnRun(FrameData& frameData) = 0;

        virtual void OnUpdate(FrameData& frameData)        = 0;
        virtual bool OnPrepareRender(FrameData& frameData) = 0;
        virtual bool OnRender(FrameData& frameData)        = 0;

        virtual void OnResize(u16 width, u16 height) = 0;

        virtual void OnShutdown() = 0;

        virtual void OnLibraryLoad()   = 0;
        virtual void OnLibraryUnload() = 0;

        friend Engine;

    protected:
        ApplicationState* m_appState = nullptr;

        UIConsole* m_pConsole                  = nullptr;
        const Engine* m_pEngine                = nullptr;
    };

    Application* CreateApplication();
    void InitApplication(Engine* engine);
    void DestroyApplication();
}  // namespace C3D
