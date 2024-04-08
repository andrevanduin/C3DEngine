
#pragma once
#include "application.h"
#include "core/clock.h"
#include "core/frame_data.h"
#include "defines.h"
#include "logger.h"
#include "renderer/renderer_types.h"
#include "systems/fonts/font_system.h"

namespace C3D
{
    class Application;
    class UIConsole;
    struct EventContext;

    struct Clocks
    {
        Clock prepareFrame;
        Clock onUpdate;
        Clock prepareRender;
        Clock onRender;
        Clock present;
        Clock total;
    };

    struct EngineState
    {
        bool running     = false;
        bool suspended   = false;
        bool initialized = false;

        /** @brief Struct containing all the different clocks we need to keep track off. */
        Clocks clocks;

        /** @brief Indicates if the window is currently being resized. */
        bool resizing = false;
        /** @brief The number of frames since last resize. Only set if resizing == true; Otherwise it's 0. */
        u8 framesSinceResize = 0;

        u16 windowWidth  = 0;
        u16 windowHeight = 0;

        f64 lastTime = 0;
    };

    class C3D_API Engine
    {
    public:
        explicit Engine(Application* pApplication, UIConsole* pConsole);

        bool Init();

        void Run();
        void Quit();
        void Shutdown();

        void OnUpdate();
        void OnResize(u32 width, u32 height);

        u16 GetWindowWidth() const { return m_state.windowWidth; }
        u16 GetWindowHeight() const { return m_state.windowHeight; }

        void OnApplicationLibraryReload(Application* app);

        const SystemManager* GetSystemsManager() const { return &m_systemsManager; }

        const LinearAllocator* GetFrameAllocator() const { return &m_frameAllocator; };

    protected:
        Application* m_application;
        UIConsole* m_pConsole;

    private:
        bool OnResizeEvent(u16 code, void* sender, const EventContext& context);
        bool OnQuitEvent(u16 code, void* sender, const EventContext& context);

        bool OnMinimizeEvent(u16 code, void* sender, const EventContext& context);
        bool OnFocusGainedEvent(u16 code, void* sender, const EventContext& context);

        /** @brief The Engine's internal state. */
        EngineState m_state;
        /** @brief Systems manager that can be used to access all the engine's systems. */
        SystemManager m_systemsManager;
        /** @brief Allocator used for allocating frame data. Gets cleared on every frame. */
        LinearAllocator m_frameAllocator;
        /** @brief The data that is relevant for every frame. */
        FrameData m_frameData;
        /** @brief A pointer to the systems manager (just used to ensure we can make macro calls in the engine.cpp). */
        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D