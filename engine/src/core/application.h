
#pragma once
#include "containers/string.h"
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
        /** @brief Center the window horizontally. When this flag is set the x property is ignored. */
        ApplicationFlagWindowCenterHorizontal = 0x1,
        /** @brief Center the window vertically. When this flag is set the y property is ignored. */
        ApplicationFlagWindowCenterVertical = 0x2,
        /** @brief Center the window horizontally and vertically. When this flag is set the x and y property are ignored. */
        ApplicationFlagWindowCenter = 0x4,
        /** @brief Make the window automatically size to the entire screen during startup.
         * When this flag is set width and height are ignored. */
        ApplicationFlagWindowFullScreen = 0x8,
    };

    using ApplicationFlagBits = u8;

    struct ApplicationState
    {
        String name;

        /** @brief The horizontal position of the window (can be negative for multi monitor setups). */
        i32 x = 0;
        /** @brief The vertical position of the window (can be negative for multi monitor setups). */
        i32 y = 0;
        /** @brief The width of the window. */
        i32 width = 0;
        /** @brief The height of the window. */
        i32 height = 0;
        /** @brief Flags that indicate certain properties about this application. */
        ApplicationFlagBits flags = ApplicationFlagNone;

        RendererPlugin* rendererPlugin = nullptr;
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

        virtual void OnResize() = 0;

        virtual void OnShutdown() = 0;

        virtual void OnLibraryLoad()   = 0;
        virtual void OnLibraryUnload() = 0;

        const SystemManager* GetSystemsManager() const { return m_pSystemsManager; }

        friend Engine;

    protected:
        ApplicationState* m_appState = nullptr;

        UIConsole* m_pConsole                  = nullptr;
        const SystemManager* m_pSystemsManager = nullptr;
        const Engine* m_pEngine                = nullptr;
    };

    Application* CreateApplication();
    void InitApplication(Engine* engine);
    void DestroyApplication();
}  // namespace C3D
