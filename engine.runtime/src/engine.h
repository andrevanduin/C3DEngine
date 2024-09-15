
#pragma once
#include "application.h"
#include "console/console.h"
#include "defines.h"
#include "frame_data.h"
#include "renderer/renderer_types.h"
#include "systems/fonts/font_system.h"
#include "time/clock.h"

namespace C3D
{
    class Application;
    struct EventContext;

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
        explicit Engine(Application* pApplication);

        bool Init();

        void Run();
        void Quit();
        void Shutdown();

        void OnUpdate();
        void OnResize(u32 width, u32 height);

        u16 GetWindowWidth() const { return m_state.windowWidth; }
        u16 GetWindowHeight() const { return m_state.windowHeight; }

        void OnApplicationLibraryReload(Application* app);

        const LinearAllocator* GetFrameAllocator() const { return &m_frameAllocator; };

    protected:
        Application* m_application;

    private:
        bool OnResizeEvent(u16 width, u16 height);

        /** @brief The Engine's internal state. */
        EngineState m_state;
        /** @brief Allocator used for allocating frame data. Gets cleared on every frame. */
        LinearAllocator m_frameAllocator;
        /** @brief The data that is relevant for every frame. */
        FrameData m_frameData;
        /** @brief The console instance. */
        UIConsole m_console;
    };
}  // namespace C3D