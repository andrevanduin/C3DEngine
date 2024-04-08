
#pragma once
#include "UI/2D/component.h"
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"
#include "core/defines.h"
#include "core/function/function.h"
#include "systems/events/event_system.h"

namespace C3D
{
    constexpr auto MAX_LINES   = 512;
    constexpr i8 MAX_HISTORY   = 64;
    constexpr auto SHOWN_LINES = 10;

    using CommandName     = CString<128>;
    using ArgName         = CString<128>;
    using CommandCallback = StackFunction<bool(const DynamicArray<ArgName>&, String&), 16>;

    class SystemManager;

    class C3D_API UIConsole
    {
        enum class LogType
        {
            Info,
            Error,
        };

    public:
        UIConsole() = default;

        void OnInit(const SystemManager* pSystemsManager);
        void OnRun();

        void OnShutDown();

        void OnUpdate();

        void RegisterCommand(const CommandName& name, const CommandCallback& func);

        void UnregisterCommand(const CommandName& name);

        void WriteLine(const char* line);

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

    private:
        bool OnResizeEvent(const u16 code, void* sender, const EventContext& context);

        void RegisterDefaultCommands();

        void WriteLineInternal(const CString<256>& line);

        bool OnKeyDownEvent(u16 code, void* sender, const EventContext& context);
        bool OnMouseScrollEvent(u16 code, void* sender, const EventContext& context);

        void OnParseCommand(const String& text);

        template <typename... Args>
        void PrintCommandMessage(const LogType type, const char* format, Args&&... args)
        {
            if (type == LogType::Info)
            {
                INSTANCE_INFO_LOG("UI_CONSOLE", format, args...);
            }
            else
            {
                INSTANCE_ERROR_LOG("UI_CONSOLE", format, args...);
            }
        }

        bool m_isOpen = false, m_initialized = false;
        bool m_isTextDirty = true, m_showCursor = false;

        f64 m_cursorTime = 0, m_scrollTime = 0;

        u32 m_startIndex = 0, m_endIndex = SHOWN_LINES, m_nextLine = 0;

        // History
        i8 m_currentHistory = -1;
        i8 m_endHistory = 0, m_nextHistory = 0;

        CircularBuffer<CString<256>, MAX_LINES> m_lines;
        CircularBuffer<CString<256>, MAX_HISTORY> m_history;

        UI_2D::ComponentHandle m_text, m_entry, m_cursor;
        UI_2D::ComponentHandle m_background;

        HashMap<CommandName, CommandCallback> m_commands;
        DynamicArray<RegisteredEventCallback> m_callbacks;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D
