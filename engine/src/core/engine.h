
#pragma once
#include "application.h"
#include "core/frame_data.h"
#include "defines.h"
#include "logger.h"
#include "renderer/render_view.h"
#include "renderer/renderer_types.h"
#include "systems/fonts/font_system.h"

namespace C3D
{
    class Application;
    class UIConsole;
    struct EventContext;

    struct EngineState
    {
        bool running     = false;
        bool suspended   = false;
        bool initialized = false;

        f64 lastTime = 0;
    };

    class C3D_API Engine final
    {
    public:
        explicit Engine(Application* pApplication, UIConsole* pConsole);

        void Init();

        void Run();
        void Quit();

        void OnUpdate();

        void OnResize(u32 width, u32 height) const;

        void GetFrameBufferSize(u32* width, u32* height) const;

        [[nodiscard]] const EngineState* GetState() const;

        void OnApplicationLibraryReload(Application* app);

        const SystemManager* GetSystemsManager() { return &m_systemsManager; }

    protected:
        Application* m_application;
        UIConsole* m_pConsole;

    private:
        void Shutdown();

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
        /* @brief The data that is relevant for every frame. */
        FrameData m_frameData;
        /** @brief A pointer to the systems manager (just used to ensure we can make macro calls in the engine.cpp). */
        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D