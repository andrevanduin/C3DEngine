
#pragma once
#include "parsers/cson_types.h"
#include "platform/platform_types.h"
#include "renderer/renderer_plugin.h"
#include "string/string.h"
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

    struct ApplicationConfig
    {
        /** @brief The name of the application. */
        String name;
        /** @brief The size that should be allocated for the per-frame allocator. */
        u64 frameAllocatorSize = 0;
        /** @brief Flags that indicate certain properties about this application. */
        ApplicationFlagBits flags = ApplicationFlagNone;
        /** @brief An array of window configs. */
        DynamicArray<WindowConfig> windowConfigs;
        /** @brief A Hashmap containing CSONObjects with the configuration for a system. Indexable by the name of the system. */
        HashMap<String, CSONObject> systemConfigs;
    };

    /** @brief An empty struct to hold the ApplicationState that can be defined by the user. */
    struct ApplicationState
    {
    };

    class C3D_API Application
    {
    public:
        Application(ApplicationState* state);

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

    private:
        void ParseWindowConfig(const CSONObject& config);

    protected:
        ApplicationConfig m_appConfig;

        UIConsole* m_pConsole   = nullptr;
        const Engine* m_pEngine = nullptr;
    };

    Application* CreateApplication();
    void InitApplication(Engine* engine);
    void DestroyApplication();
}  // namespace C3D
