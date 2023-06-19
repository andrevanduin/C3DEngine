
#include "console.h"

#include "core/engine.h"
#include "core/string_utils.h"
#include "renderer/render_view.h"
#include "systems/cvars/cvar_system.h"
#include "systems/events/event_system.h"
#include "systems/input/input_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    UIConsole::UIConsole() : m_logger("CONSOLE") {}

    void UIConsole::OnInit(const SystemManager* pSystemsManager)
    {
        m_pSystemsManager = pSystemsManager;

        m_text.Create(m_pSystemsManager, UITextType::Bitmap, "Ubuntu Mono 21px", 21, "-");
        m_entry.Create(m_pSystemsManager, UITextType::Bitmap, "Ubuntu Mono 21px", 21, " ");
        m_cursor.Create(m_pSystemsManager, UITextType::Bitmap, "Ubuntu Mono 21px", 21, "|");

        m_text.SetPosition({5, 5, 0});
        m_entry.SetPosition({5, 30, 0});
        m_cursor.SetPosition({5, 30, 0});

        Event.Register(EventCodeKeyDown, [this](const u16 code, void* sender, const EventContext& context) {
            return OnKeyDownEvent(code, sender, context);
        });
        Event.Register(EventCodeMouseScrolled, [this](const u16 code, void* sender, const EventContext& context) {
            return OnMouseScrollEvent(code, sender, context);
        });

        m_commands.Create(377);
        m_initialized = true;

        RegisterDefaultCommands();
    }

    void UIConsole::OnShutDown()
    {
        if (m_initialized)
        {
            // Clear our commands
            m_commands.Destroy();
            m_text.Destroy();
            m_entry.Destroy();
            m_cursor.Destroy();

            m_initialized = false;
        }
    }

    void UIConsole::OnUpdate()
    {
        if (Input.IsKeyPressed(KeyGrave))
        {
            m_isOpen ^= true;
            m_logger.Info(m_isOpen ? "Opened" : "Closed");
        }

        if (m_isOpen)
        {
            m_cursorCounter++;
            m_cursorCounter %= 180;

            if (m_scrollCounter > 0) m_scrollCounter--;
        }
    }

    void UIConsole::OnRender(UIPacketData& packet)
    {
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

            m_text.SetText(buffer.Data());
            m_entry.SetPosition({5, m_text.GetMaxY() + 5, 0});
            m_isTextDirty = false;
        }

        if (m_isEntryDirty)
        {
            m_entry.SetText(m_current.Data());
            m_isEntryDirty = false;
        }

        m_cursor.SetPosition({m_entry.GetMaxX(), m_text.GetMaxY() + 5, 0});

        if (m_isOpen)
        {
            packet.texts.PushBack(&m_text);

            if (!m_current.Empty()) packet.texts.PushBack(&m_entry);
            if (m_cursorCounter >= 90) packet.texts.PushBack(&m_cursor);
        }
    }

    void UIConsole::RegisterCommand(const CommandName& name, const CommandCallback& func)
    {
        m_commands.Set(name, func);
        m_logger.Info("Registered command: \'{}\'", name);
    }

    void UIConsole::UnregisterCommand(const CommandName& name)
    {
        if (m_initialized)
        {
            if (m_commands.Has(name))
            {
                m_commands.Delete(name);
                m_logger.Info("UnRegistered command: \'{}\'", name);
            }
            else
            {
                m_logger.Warn("No command with name \'{}\' is registered", name);
            }
        }
    }

    void UIConsole::WriteLine(const char* line)
    {
        const auto len = std::strlen(line);
        CString<256> currentLine = "";
        auto currentLineLength = 0;

        for (size_t i = 0; i < len; i++)
        {
            const auto character = line[i];

            if (character == '\n' || currentLineLength >= 255)
            {
                WriteLineInternal(currentLine);
                currentLine = "";
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
        m_endIndex = m_nextLine;

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

            m_isEntryDirty = true;
            m_current = m_history[m_currentHistory];
            return true;
        }

        if (keyCode == KeyArrowDown)
        {
            if (m_currentHistory != -1 && m_currentHistory < m_nextHistory)
            {
                m_currentHistory++;
                m_isEntryDirty = true;
                m_current = m_history[m_currentHistory];
                return true;
            }

            return false;
        }

        if (keyCode == KeyEnter)
        {
            return OnParseCommand();
        }
        if (keyCode == KeyBackspace)
        {
            if (!m_current.Empty())
            {
                m_current[m_current.Size() - 1] = '\0';
                m_isEntryDirty = true;
            }
            return true;
        }

        const auto shiftHeld = Input.IsShiftHeld();
        char typedChar;

        if (keyCode >= KeyA && keyCode <= KeyZ)
        {
            // Characters
            typedChar = static_cast<char>(keyCode);
            if (shiftHeld) typedChar -= 32;  // If shift is held we want capital letters
        }
        else if (keyCode == KeySpace)
        {
            typedChar = static_cast<char>(keyCode);
        }
        else if (keyCode == KeyMinus || keyCode == KeyEquals)
        {
            typedChar = static_cast<char>(keyCode);
            if (shiftHeld)
            {
                switch (keyCode)
                {
                    case KeyMinus:
                        typedChar = '_';
                        break;
                    case KeyEquals:
                        typedChar = '+';
                        break;
                    default:
                        m_logger.Fatal("Unknown char found while trying to parse key for console {}", keyCode);
                        break;
                }
            }
        }
        else if (keyCode >= Key0 && keyCode <= Key9)
        {
            // Numbers
            typedChar = static_cast<char>(keyCode);
            if (shiftHeld)
            {
                switch (keyCode)
                {
                    case Key0:
                        typedChar = ')';
                        break;
                    case Key1:
                        typedChar = '!';
                        break;
                    case Key2:
                        typedChar = '@';
                        break;
                    case Key3:
                        typedChar = '#';
                        break;
                    case Key4:
                        typedChar = '$';
                        break;
                    case Key5:
                        typedChar = '%';
                        break;
                    case Key6:
                        typedChar = '^';
                        break;
                    case Key7:
                        typedChar = '&';
                        break;
                    case Key8:
                        typedChar = '*';
                        break;
                    case Key9:
                        typedChar = '(';
                        break;
                    default:
                        m_logger.Fatal("Unknown char found while trying to parse numbers for console {}", keyCode);
                        break;
                }
            }
        }
        else
        {
            // A key was pressed that we don't care about
            return false;
        }

        m_current.Append(typedChar);
        m_isEntryDirty = true;
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
        if (scrollAmount > 0 && m_scrollCounter == 0 && m_startIndex > minStartIndex)
        {
            m_scrollCounter = 10;
            m_startIndex--;
            m_endIndex--;
            m_isTextDirty = true;
        }
        if (scrollAmount < 0 && m_scrollCounter == 0 && m_endIndex < m_nextLine)
        {
            m_scrollCounter = 10;
            m_startIndex++;
            m_endIndex++;
            m_isTextDirty = true;
        }
        return true;
    }

    bool UIConsole::OnParseCommand()
    {
        if (StringUtils::IsEmptyOrWhitespaceOnly(m_current))
        {
            m_current = "";
            m_isEntryDirty = true;
            return true;
        }

        m_history[m_nextHistory] = m_current;
        m_nextHistory++;
        m_endHistory = m_nextHistory;
        m_currentHistory = -1;

        const auto args = StringUtils::Split<256, 128>(m_current, ' ');
        if (args.Empty())
        {
            PrintCommandMessage(LogType::Error, "The input: \'{}\' failed to be parsed!", m_current);
            return false;
        }

        // The first argument is the command
        const auto commandName = args[0];
        // Check if the command actually exists
        if (!m_commands.Has(commandName))
        {
            // Not a command let's warn the user
            PrintCommandMessage(LogType::Error, "The command: \'{}\' does not exist!", commandName);
            return false;
        }

        // A valid command let's try to run the associated logic
        const CommandCallback& command = m_commands[commandName];
        if (String output = ""; !command.operator()(args, output))
        {
            PrintCommandMessage(LogType::Error, "The command \'{}\' failed to execute:", commandName);
            m_logger.Error(output.Data());
        }
        else
        {
            PrintCommandMessage(LogType::Info, "The command \'{}\' executed successfully:", commandName);
            m_logger.Info(output.Data());
        }

        m_current = "";
        m_isEntryDirty = true;
        return true;
    }

    bool UIConsole::IsInitialized() const { return m_initialized; }

    bool UIConsole::IsOpen() const { return m_isOpen; }

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
