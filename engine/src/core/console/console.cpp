
#include "console.h"

#include "core/input.h"
#include "core/events/event.h"
#include "core/events/event_callback.h"
#include "renderer/render_view.h"
#include "resources/font.h"
#include "services/system_manager.h"

namespace C3D
{
	UIConsole::UIConsole()
		: m_isOpen(false), m_initialized(false), m_isTextDirty(true), m_isEntryDirty(true), m_cursorCounter(0), m_scrollCounter(0),
		  m_startIndex(0), m_endIndex(SHOWN_LINES), m_nextLine(0), m_logger("CONSOLE")
	{}

	void UIConsole::OnInit()
	{
		m_text.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, "-");
		m_entry.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, " ");
		m_cursor.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, "|");

		m_text.SetPosition(	 { 5,  5, 0	});
		m_entry.SetPosition( { 5, 30, 0 });
		m_cursor.SetPosition({ 5, 30, 0 });

		Event.Register(KeyDown, new EventCallback(this, &UIConsole::OnKeyDownEvent));

		m_commands.Create(377);

		m_initialized = true;
	}

	void UIConsole::OnShutDown()
	{
		if (m_initialized)
		{
			m_commands.Destroy();

			Event.UnRegister(KeyDown, new EventCallback(this, &UIConsole::OnKeyDownEvent));

			m_text.Destroy();
			m_entry.Destroy();
			m_cursor.Destroy();
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

			if (Input.IsKeyDown(KeyArrowUp) && m_scrollCounter == 0)
			{
				m_scrollCounter = 10;

				if (m_startIndex % MAX_LINES > 0)
				{
					m_startIndex--;
					m_endIndex--;
					m_isTextDirty = true;
				}
			}
			if (Input.IsKeyDown(KeyArrowDown) && m_scrollCounter == 0)
			{
				m_scrollCounter = 20;

				if (m_endIndex < m_nextLine)
				{
					m_startIndex++;
					m_endIndex++;
					m_isTextDirty = true;
				}
			}
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
			m_entry.SetPosition({ 5, m_text.GetMaxY() + 5, 0});
			m_isTextDirty = false;
		}

		if (m_isEntryDirty)
		{
			m_entry.SetText(m_current.Data());
			m_isEntryDirty = false;
		}

		m_cursor.SetPosition({ m_entry.GetMaxX(), m_text.GetMaxY() + 5, 0});

		if (m_isOpen)
		{
			packet.texts.PushBack(&m_text);

			if (!m_current.Empty())		packet.texts.PushBack(&m_entry);
			if (m_cursorCounter >= 90)	packet.texts.PushBack(&m_cursor);
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
		if (keyCode == KeyEnter)
		{
			if (!m_commands.Has(m_current))
			{
				CString<256> text = "The command: \'";
				text += m_current;
				text += "\' does not exist!";
				WriteLineInternal(text);
			}
			else
			{
				// WriteLineInternal(m_current);
				m_current = "";
			}
			return true;
		}

		if (keyCode == KeyBackspace)
		{
			if (!m_current.Empty()) m_current[m_current.Size() - 1] = '\0';
			return true;
		}

		const auto shiftHeld = Input.IsShiftHeld();
		char typedChar = '\0';

		if (keyCode >= KeyA && keyCode <= KeyZ)
		{
			// Characters
			typedChar = static_cast<char>(keyCode);
			if (shiftHeld) typedChar -= 32; // If shift is held we want capital letters
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

	bool UIConsole::IsInitialized() const
	{
		return m_initialized;
	}

	bool UIConsole::IsOpen() const
	{
		return m_isOpen;
	}
}
