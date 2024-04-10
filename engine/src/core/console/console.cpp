
#include "console.h"

#include "core/engine.h"
#include "core/string_utils.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/cvars/cvar_system.h"
#include "systems/events/event_system.h"
#include "systems/fonts/font_system.h"
#include "systems/input/input_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "UI_CONSOLE";
    constexpr auto BLINK_TIME           = 0.9f;
    constexpr auto SCROLL_DELAY         = 0.1f;

    void UIConsole::OnInit(const SystemManager* pSystemsManager)
    {
        m_pSystemsManager = pSystemsManager;

        m_commands.Create(377);
        m_initialized = true;

        RegisterDefaultCommands();

        m_callbacks.PushBack(Event.Register(EventCodeKeyDown, [this](const u16 code, void* sender, const EventContext& context) {
            return OnKeyDownEvent(code, sender, context);
        }));
        m_callbacks.PushBack(Event.Register(EventCodeMouseScrolled, [this](const u16 code, void* sender, const EventContext& context) {
            return OnMouseScrollEvent(code, sender, context);
        }));
        m_callbacks.PushBack(Event.Register(EventCodeResized, [this](const u16 code, void* sender, const EventContext& context) {
            return OnResizeEvent(code, sender, context);
        }));
    }

    void UIConsole::OnRun()
    {
        auto font = Fonts.Acquire("Ubuntu Mono 21px", FontType::Bitmap, 32);

        auto windowSize = OS.GetWindowSize();

        m_background = UI2D.AddPanel(u16vec2(0, 0), u16vec2(windowSize.x, 100), u16vec2(16, 16));
        m_text       = UI2D.AddLabel(u16vec2(5, 5), "-", font);
        m_entry      = UI2D.AddTextbox(u16vec2(5, 5), u16vec2(windowSize.x - 10, 30), "", font);

        UI2D.AddOnEndTextInputHandler(m_entry, [this](u16 key, const String& text) {
            if (key == KeyEnter)
            {
                // Parse the inputted text as a command
                OnParseCommand(text);
                // Default behaviour is to set control to inactive after KeyEnter so we set it back to active
                UI2D.SetActive(m_entry, true);
            }
        });

        UI2D.MakeVisible(m_background, false);
        UI2D.MakeVisible(m_text, false);
        UI2D.MakeVisible(m_entry, false);
    }

    void UIConsole::OnShutDown()
    {
        if (m_initialized)
        {
            for (auto& callback : m_callbacks)
            {
                Event.Unregister(callback);
            }
            m_callbacks.Destroy();

            // Clear our commands
            m_commands.Destroy();
            m_initialized = false;
        }
    }

    void UIConsole::OnUpdate()
    {
        if (Input.IsKeyPressed(KeyGrave))
        {
            m_isOpen ^= true;
            INFO_LOG(m_isOpen ? "Opened" : "Closed");

            UI2D.MakeVisible(m_text, m_isOpen);
            UI2D.MakeVisible(m_entry, m_isOpen);
            UI2D.MakeVisible(m_background, m_isOpen);
            UI2D.SetActive(m_entry, m_isOpen);
        }

        f32 textMaxY = UI2D.GetTextMaxY(m_text);
        if (m_isTextDirty)
        {
            constexpr auto maxChars = 4096;
            CString<maxChars> buffer;

            for (u32 i = m_startIndex; i < m_endIndex; i++)
            {
                const auto& line = m_lines[i];

                buffer += line;
                buffer += '\n';
            }

            UI2D.SetText(m_text, buffer.Data());
            UI2D.SetPosition(m_entry, vec2(5, textMaxY + 15));
            UI2D.SetHeight(m_background, textMaxY + 50);

            m_isTextDirty = false;
        }
    }

    void UIConsole::RegisterCommand(const CommandName& name, const CommandCallback& func)
    {
        m_commands.Set(name, func);
        INFO_LOG("Registered command: '{}'.", name);
    }

    void UIConsole::UnregisterCommand(const CommandName& name)
    {
        if (m_initialized)
        {
            if (m_commands.Has(name))
            {
                m_commands.Delete(name);
                INFO_LOG("UnRegistered command: '{}'.", name);
            }
            else
            {
                WARN_LOG("No command with name '{}' is registered.", name);
            }
        }
    }

    void UIConsole::WriteLine(const char* line)
    {
        const auto len           = std::strlen(line);
        CString<256> currentLine = "";
        auto currentLineLength   = 0;

        for (size_t i = 0; i < len; i++)
        {
            const auto character = line[i];

            if (character == '\n' || currentLineLength >= 255)
            {
                WriteLineInternal(currentLine);
                currentLine       = "";
                currentLineLength = 0;
            }
            else
            {
                currentLine += character;
                currentLineLength++;
            }
        }

        if (!currentLine.Empty())
        {
            WriteLineInternal(currentLine);
        }
    }

    void UIConsole::WriteLineInternal(const CString<256>& line)
    {
        m_lines[m_nextLine] = line;
        m_nextLine++;

        m_startIndex = m_nextLine - SHOWN_LINES;
        m_endIndex   = m_nextLine;

        m_isTextDirty = true;
    }

    bool UIConsole::OnKeyDownEvent(u16 code, void* sender, const EventContext& context)
    {
        if (!m_isOpen) return false;

        const u16 keyCode = context.data.u16[0];
        if (keyCode == KeyArrowUp)
        {
            // Our minimum start index value depends on if we wrapped around in our circular buffer yet
            // > If we have not wrapped around the value is simply 0 (first index into our circular buffer)
            // > If we did wrap around it must be larger than the end index % MAX_HISTORY since that is the end index of
            // were we store our lines. Everything beyond that index is actually the wrapped around newer lines
            const i8 minStartIndex = m_endHistory > MAX_HISTORY ? m_endHistory % MAX_HISTORY : 0;
            if (m_currentHistory > minStartIndex)
            {
                m_currentHistory--;
            }
            else if (m_currentHistory == -1 && m_endHistory != 0)
            {
                m_currentHistory = m_endHistory - 1;
            }

            if (m_currentHistory == -1)
            {
                // There is no history yet
                return false;
            }

            UI2D.SetText(m_entry, m_history[m_currentHistory].Data());
            return true;
        }

        if (keyCode == KeyArrowDown)
        {
            if (m_currentHistory != -1 && m_currentHistory < m_nextHistory)
            {
                m_currentHistory++;
                UI2D.SetText(m_entry, m_history[m_currentHistory].Data());
                return true;
            }

            return false;
        }

        return true;
    }

    bool UIConsole::OnMouseScrollEvent(u16 code, void* sender, const EventContext& context)
    {
        if (!m_isOpen) return false;

        const auto scrollAmount = context.data.i8[0];

        // Our minimum start index value depends on if we wrapped around in our circular buffer yet
        // > If we have not wrapped around the value is simply 0 (first index into our circular buffer)
        // > If we did wrap around it must be larger than the end index % MAX_LINES since that is the end index of
        // were we store our lines. Everything beyond that index is actually the wrapped around newer lines
        const auto minStartIndex = m_endIndex > MAX_LINES ? m_endIndex % MAX_LINES : 0;

        auto currentTime = OS.GetAbsoluteTime();
        if (scrollAmount > 0 && currentTime >= m_scrollTime && m_startIndex > minStartIndex)
        {
            m_scrollTime = currentTime + SCROLL_DELAY;
            m_startIndex--;
            m_endIndex--;
            m_isTextDirty = true;
        }
        if (scrollAmount < 0 && currentTime >= m_scrollTime && m_endIndex < m_nextLine)
        {
            m_scrollTime = currentTime + SCROLL_DELAY;
            m_startIndex++;
            m_endIndex++;
            m_isTextDirty = true;
        }

        return true;
    }

    void UIConsole::OnParseCommand(const String& text)
    {
        if (text.EmptyOrWhitespace())
        {
            return;
        }

        auto current = CString<256>(text.Data());

        m_history[m_nextHistory] = current;
        m_nextHistory++;
        m_endHistory     = m_nextHistory;
        m_currentHistory = -1;

        const auto args = StringUtils::Split<256, 128>(current, ' ');
        if (args.Empty())
        {
            ERROR_LOG("The input: \'{}\' failed to be parsed!", current);
            UI2D.SetText(m_entry, "");
            return;
        }

        // The first argument is the command
        const auto commandName = args[0];
        // Check if the command actually exists
        if (!m_commands.Has(commandName))
        {
            // Not a command let's warn the user
            ERROR_LOG("The command: '{}' does not exist!", commandName);
            UI2D.SetText(m_entry, "");
            return;
        }

        // A valid command let's try to run the associated logic
        const CommandCallback& command = m_commands[commandName];
        if (String output = ""; !command.operator()(args, output))
        {
            ERROR_LOG("The command '{}' failed to execute:", commandName);
            if (!output.Empty())
            {
                const char* msg = output.Data();
                ERROR_LOG(msg);
            }
        }
        else
        {
            INFO_LOG("The command '{}' executed successfully:", commandName);
            if (!output.Empty())
            {
                const char* msg = output.Data();
                INFO_LOG(msg);
            }
        }

        UI2D.SetText(m_entry, "");
    }

    bool UIConsole::IsInitialized() const { return m_initialized; }

    bool UIConsole::IsOpen() const { return m_isOpen; }

    bool UIConsole::OnResizeEvent(const u16 code, void* sender, const EventContext& context)
    {
        u16 width = context.data.u16[0];
        UI2D.SetWidth(m_background, width);
        UI2D.SetWidth(m_entry, width - 10);

        // Let other also handle this event
        return false;
    }

    void UIConsole::RegisterDefaultCommands()
    {
        RegisterCommand("exit", [this](const DynamicArray<ArgName>&, String& output) {
            Event.Fire(EventCodeApplicationQuit, nullptr, {});
            output += "Shutting down";
            return true;
        });
        CVars.RegisterDefaultCommands();
    }
}  // namespace C3D
