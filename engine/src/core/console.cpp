
#include "console.h"

#include "input.h"
#include "renderer/render_view.h"
#include "services/system_manager.h"

namespace C3D
{
	UIConsole::UIConsole()
		: m_isOpen(false), m_isDirty(true), m_initialized(false), m_startIndex(0),
		  m_endIndex(10), m_nextLine(0), m_logger("CONSOLE")
	{}

	void UIConsole::OnInit()
	{
		m_text.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, "-");
		// m_entry.Create(UITextType::Bitmap, "Ubuntu Mono 21px", 21, "");

		m_text.SetPosition({ 5, 5, 0 });
		// m_entry.SetPosition({ 10, 30, 0 });

		m_initialized = true;
	}

	void UIConsole::OnShutDown()
	{
		if (m_initialized)
		{
			m_text.Destroy();
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
			if (Input.IsKeyPressed(KeyUp))
			{
				if (m_startIndex > 0)
				{
					m_startIndex--;
					m_endIndex--;
					m_isDirty = true;
				}
			}
			if (Input.IsKeyPressed(KeyDown))
			{
				if (m_endIndex < m_usedLines)
				{
					m_startIndex++;
					m_endIndex++;
					m_isDirty = true;
				}
			}
		}
	}

	void UIConsole::OnRender(UIPacketData& packet)
	{
		if (m_isDirty)
		{
			std::lock_guard lock(m_mutex);

			constexpr auto maxLineCount = 16384;
			CString<maxLineCount> buffer;

			for (u32 i = m_startIndex; i < m_endIndex; i++)
			{
				const auto& line = m_lines[i];

				buffer += line;
				buffer += '\n';
			}

			m_text.SetText(buffer.Data());
			m_isDirty = false;
		}

		if (m_isOpen)
		{
			packet.texts.PushBack(&m_text);
		}
	}

	void UIConsole::WriteLine(const char* line)
	{
		std::lock_guard lock(m_mutex);

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
		// If we exceed the available lines we just wrap around
		if (m_nextLine >= MAX_LINES)
		{
			m_nextLine = 0;
		}

		m_lines[m_nextLine] = line;
		m_nextLine++;

		m_usedLines++;
		if (m_usedLines > MAX_LINES) m_usedLines = MAX_LINES;

		m_isDirty = true;
	}

	bool UIConsole::IsInitialized() const
	{
		return m_initialized;
	}

	bool UIConsole::IsOpen() const
	{
		return m_isOpen;
	}

	CString<256>& UIConsole::GetCurrentLine()
	{
		if (m_nextLine == 0)
		{
			return m_lines[m_nextLine];
		}
		return m_lines[m_nextLine - 1];
	}
}
