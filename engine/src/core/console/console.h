
#pragma once
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"
#include "core/defines.h"
#include "core/function/function.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"

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
        void OnShutDown();

        void OnUpdate();
        void OnRender(UIPacketData& packet);

        void RegisterCommand(const CommandName& name, const CommandCallback& func);

        void UnregisterCommand(const CommandName& name);

        void WriteLine(const char* line);

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

    private:
        void RegisterDefaultCommands();

        void WriteLineInternal(const CString<256>& line);

        bool OnKeyDownEvent(u16 code, void* sender, const EventContext& context);
        bool OnMouseScrollEvent(u16 code, void* sender, const EventContext& context);

        bool OnParseCommand();

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

            m_current      = "";
            m_isEntryDirty = true;
        }

        bool m_isOpen = false, m_initialized = false;
        bool m_isTextDirty = true, m_isEntryDirty = true;

        u8 m_cursorCounter = 0, m_scrollCounter = 0;
        u32 m_startIndex = 0, m_endIndex = SHOWN_LINES, m_nextLine = 0;

        // History
        i8 m_currentHistory = -1;
        i8 m_endHistory = 0, m_nextHistory = 0;

        CircularBuffer<CString<256>, MAX_LINES> m_lines;
        CircularBuffer<CString<256>, MAX_HISTORY> m_history;

        UIText m_text, m_entry, m_cursor;
        CString<256> m_current;

        HashMap<CommandName, CommandCallback> m_commands;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D
